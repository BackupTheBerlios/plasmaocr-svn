#include "common.h"
#include "core.h"
#include "library.h"
#include "chaincode.h"
#include "editdist.h"
#include "shiftcut.h"
#include "bitmaps.h"
#include "wordcut.h"
#include "pattern.h"
#include <assert.h>
#include <string.h>


struct CoreStruct
{
    int libraries_count, libraries_allocated;
    Library *libraries;
    int matches_count, matches_allocated;
    Match *matches;
    int records_count, records_allocated;
    LibraryRecord **records;
    int orange_policy;
    Library orange_library;
};

static void init_libraries_list(Core c)
    LIST_CREATE(Library, c->libraries, c->libraries_count, c->libraries_allocated, 8)

static Library *append_library(Core c)
    LIST_APPEND(Library, c->libraries, c->libraries_count, c->libraries_allocated)

static void init_matches_list(Core c)
    LIST_CREATE(Match, c->matches, c->matches_count, c->matches_allocated, 16)

static Match *append_match(Core c)
    LIST_APPEND(Match, c->matches, c->matches_count, c->matches_allocated)

static void init_records_list(Core c)
    LIST_CREATE(LibraryRecord *, c->records, c->records_count, c->records_allocated, 16)

static LibraryRecord **append_record(Core c)
    LIST_APPEND(LibraryRecord *, c->records, c->records_count, c->records_allocated)


void set_core_orange_policy(Core c, int level)
{
    c->orange_policy = level;
    if (level)
        c->orange_library = library_create();
}

Library get_core_orange_library(Core c)
{
    return c->orange_library;
}


Core create_core()
{
    Core c = MALLOC1(struct CoreStruct);
    init_libraries_list(c);
    init_matches_list(c);
    init_records_list(c);
    c->orange_policy = 0;
    c->orange_library = NULL;
    return c;
}

void free_core(Core c)
{
    int i;

    for (i = 0; i < c->libraries_count; i++)
        library_free(c->libraries[i]);

    if (c->orange_library)
        library_free(c->orange_library);
    FREE(c->libraries);
    FREE(c->matches);
    FREE(c->records);
    FREE1(c);
}

void add_to_core(Core c, Library l)
{
    *(append_library(c)) = l;
}


/* Return 0 if all the library samples read the same.
 */
static int samples_conflict(LibraryRecord **s, int n)
{
    int i;
    assert(n > 0);

    for (i = 1; i < n; i++)
    {
        if (strcmp(s[0]->text, s[i]->text))
            return 1;
    }
    return 0;
}


static RecognizedLetter *create_recognized_letter(char *text, ColorCode c)
{
    RecognizedLetter *result = MALLOC1(RecognizedLetter);
    if (text)
    {
        result->text = MALLOC(char, strlen(text) + 1);
        strcpy(result->text, text);
    }
    else
        result->text = NULL;
    result->color = c;
    result->best_match = NULL;
    return result;
}


static void iterate_core(Core c, LibraryIterator *iter)
{
    library_iterator_init(iter, c->libraries_count, c->libraries);
}


static RecognizedLetter *shiftcut_recognize(Core c, Pattern p, int need_explanation)
{
    char *best = NULL;
    long best_dist = 0x7FFFFFFFL;
    LibraryIterator best_iter;
    LibraryIterator iter;
    LibraryRecord *rec;
    RecognizedLetter *result;

    iterate_core(c, &iter);

    while ((rec = library_iterator_next(&iter)))
    {
        long dist = patterns_shiftcut_dist(rec->pattern, p);

        if (dist < best_dist)
        {
            best_dist = dist;
            best = rec->text;
            best_iter = iter;
        }
    }

    result = create_recognized_letter(best, (best ? CC_YELLOW : CC_RED));

    if (need_explanation)
    {
        result->best_match = MALLOC1(LibraryIterator);
        *(result->best_match) = best_iter;
    }

    return result;
}


RecognizedLetter *recognize_pattern(Core c, Pattern p, int need_explanation)
{
    /* First, match to everything */

    int i;
    LibraryRecord *rec;
    LibraryIterator iter;
    RecognizedLetter *result;

    c->matches_count = 0;
    c->records_count = 0;
    iterate_core(c, &iter);

    while ((rec = library_iterator_next(&iter)))
    {
        Match m;
        if (rec->text[0] == '\0') continue;
        m = match_patterns(rec->pattern, p);
        assert(rec->pattern);
        if (m)
        {
            * (append_match(c)) = m;
            * (append_record(c)) = rec;
        }
    }

    if (!c->matches_count)
        result = create_recognized_letter(NULL, CC_RED);
    else
    {
        Match *good_matches = MALLOC(Match, c->matches_count);
        LibraryRecord **good_samples = MALLOC(LibraryRecord *, c->matches_count);
        int good_matches_found = 0;
        int best_match = -1;
        int best_penalty = -1;
        int penalty;

        /* Another pass over matches, this time with ED-comparison */
        for (i = 0; i < c->matches_count; i++)
        {
            if (compare_patterns(c->records[i]->radius, c->matches[i], c->records[i]->pattern, p, &penalty))
            {
                good_matches[good_matches_found] = c->matches[i];
                good_samples[good_matches_found] = c->records[i];
                good_matches_found++;
            }

            if (i == 0 || penalty < best_penalty)
            {
                best_match = i;
                best_penalty = penalty;
            }
        }

        if (!good_matches_found)
            result = create_recognized_letter(c->records[best_match]->text, CC_YELLOW);
        else if (samples_conflict(good_samples, good_matches_found))
            result = create_recognized_letter(c->records[best_match]->text, CC_BLUE);
        else
            result = create_recognized_letter(good_samples[0]->text, CC_GREEN);

        FREE(good_matches);
        FREE(good_samples);
    }

    if (result->color == CC_RED || result->color == CC_YELLOW)
    {
        RecognizedLetter *alternative = shiftcut_recognize(c, p, need_explanation);
        if (result)
            free_recognized_letter(result);

        result = alternative;
    }

    for (i = 0; i < c->matches_count; i++)
        destroy_match(c->matches[i]);

    return result;
}



RecognizedLetter *recognize_letter(Core c,
                                   unsigned char **pixels, int width, int height,
                                   int need_explanation)
{
    Pattern p = create_pattern(pixels, width, height);
    RecognizedLetter *result = recognize_pattern(c, p, need_explanation);
    int cc = result->color;
    if (c->orange_policy && (cc == CC_RED || cc == CC_YELLOW))
    {
        Shelf *s = shelf_create(c->orange_library);
        LibraryRecord *rec = shelf_append(s);

        s->pixels = copy_bitmap(pixels, width, height);
        s->width = width;
        s->height = height;
        s->ownership = 1;

        rec->pattern = p;
        if (result->text)
            strncpy(rec->text, result->text, MAX_TEXT_SIZE);
        else
            rec->text[0] = '\0';
        rec->radius = DEFAULT_RADIUS;
        rec->left = 0;
        rec->top = 0;
        rec->width = width;
        rec->height = height;
    }
    else
        free_pattern(p);

    return result;
}


void free_recognized_letter(RecognizedLetter *r)
{
    if (r->text) FREE(r->text);
    if (r->best_match) FREE(r->best_match);
    FREE1(r);
}


static void build_recognized_word_text(RecognizedWord *rw)
{
    int s = 0;
    int i;

    for (i = 0; i < rw->count; i++)
        if (rw->letters[i]->text)
            s += strlen(rw->letters[i]->text);

    rw->text = MALLOC(char, s + 1);

    s = 0;
    for (i = 0; i < rw->count; i++)
    {
        if (rw->letters[i]->text)
        {
            strcpy(rw->text + s, rw->letters[i]->text);
            s += strlen(rw->letters[i]->text);
        }
    }

    rw->text[s] = '\0';
}


RecognizedWord *recognize_word(Core c,
                               unsigned char **pixels, int width, int height,
                               int need_explanation)
{
    PatternCache pc = create_pattern_cache(pixels, width, height);
    Pattern p;
    Shelf *s = NULL;
    WordCut *wc = cut_word(pixels, width, height);
    int count = wc->count + 1;  /* the number of chunks is number of cuts + 1 */
    int i;
    RecognizedWord *rw = MALLOC1(RecognizedWord);
    rw->count = count;
    rw->letters = MALLOC(RecognizedLetter *, count);

    for (i = 0; i < count; i++)
    {
        int x_beg, x_end;
        ColorCode cc;
        if (i)
            x_beg = wc->position[i-1];
        else
            x_beg = 0;

        if (i != count - 1)
            x_end = wc->position[i];
        else
            x_end = width;

        assert(x_end > x_beg);
        p = create_pattern_from_cache(pixels, width, height,
                                      x_beg, 0, x_end - x_beg, height, pc);

        rw->letters[i] = recognize_pattern(c, p, need_explanation);

        /* XXX too much code duplicated with recognize_letter */
        cc = rw->letters[i]->color;
        if (c->orange_policy && (cc == CC_RED || cc == CC_YELLOW))
        {
            LibraryRecord *rec;
            if (!s)
            {
                s = shelf_create(c->orange_library);
                s->pixels = copy_bitmap(pixels, width, height);
                s->width = width;
                s->height = height;
                s->ownership = 1;
            }

            rec = shelf_append(s);

            rec->pattern = p;
            if (rw->letters[i]->text)
                strncpy(rec->text, rw->letters[i]->text, MAX_TEXT_SIZE);
            else
                rec->text[0] = '\0';
            rec->radius = DEFAULT_RADIUS;
            rec->left = x_beg;
            rec->top = 0;
            rec->width = x_end - x_beg;
            rec->height = height;
        }
        else
            free_pattern(p);
    }

    build_recognized_word_text(rw);
    destroy_word_cut(wc);
    destroy_pattern_cache(pc);

    return rw;
}

void free_recognized_word(RecognizedWord *rw)
{
    int i;
    for (i = 0; i < rw->count; i++)
        free_recognized_letter(rw->letters[i]);

    if (rw->text) FREE(rw->text);
    FREE(rw->letters);
    FREE1(rw);
}
