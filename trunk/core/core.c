#include "common.h"
#include "chaincode.h"
#include "editdist.h"
#include "shiftcut.h"
#include "core.h"
#include "io.h"
#include "pnm.h"
#include "linewise.h"
#include <assert.h>
#include <string.h>











// _________________________________   library   ______________________________________

typedef struct
{
    char *text;
    char *path; // path to the PBM file this sample was in
    int radius;
    Pattern pattern;    
} Sample;

struct LibraryStruct
{
    int n;
    int allocated;
    Sample *samples;
};


void load_raw_sample(Sample *s, const char *text, const char *path, int radius)
{
    unsigned char **pixels;
    int width, height;
    s->text = strdup(text);
    s->path = strdup(path);
    s->radius = radius;
    if (load_pnm(path, &pixels, &width, &height) != PBM)
    {
        perror(path);
        exit(1);
    }
    
    s->pattern = core_prepare(pixels, width, height);
    core_promote(s->pattern);
}


void load_sample_by_line(Sample *s, const char *line)
{
    int n = strlen(line) + 1;
    char text[n];
    char path[n];
    int radius;
    if (sscanf(line, "%s %d %s", text, &radius, path) != 3)
    {
        fprintf(stderr, "Invalid line: %s\n", line);
        exit(1);
    }
    load_raw_sample(s, text, path, radius);
}


static int is_comment_line(const char *line)
{
    while (1) switch(*line)
    {
        case '\0': case '#':
            return 1;
        case ' ': case '\t': case '\r': case '\n': case '\f':
            line++;
        break;
        default:
            return 0;
    }
}

static Sample *append_to_library(Library l)
    LIST_APPEND(Sample, l->samples, l->n, l->allocated)

Library core_load_raw_library(const char *path)
{
    FILE *f = fopen(path, "r");
    LinewiseReader r;
    Library l;
    const char *line;
    if (!f)
    {
        perror(path);
        exit(1);
    }

    r = linewise_reader_create(f);
    l = MALLOC1(struct LibraryStruct);
    l->n = 0;
    l->allocated = 10;
    l->samples = MALLOC(Sample, l->allocated);
    
    while ((line = linewise_read_and_chomp(r)))
    {
        if (is_comment_line(line)) continue;
        load_sample_by_line(append_to_library(l), line);
    }
    
    l->samples = REALLOC(Sample, l->samples, l->n);
    return l;
}


static void save_sample(Sample *s, FILE *f)
{
    int t = strlen(s->text);
    int p = strlen(s->path);
    write_int32(t, f);
    write_int32(p, f);
    fwrite(s->text, 1, t, f);
    fwrite(s->path, 1, p, f);
    write_int32(s->radius, f);
    core_save_pattern(s->pattern, f);
}


static int load_sample(Sample *s, FILE *f)
{
    int t = read_int32(f);
    int p = read_int32(f);
    if (feof(f)) return 0;
    if (t < 0 || t > 10000 || p < 0 || p > 10000)
    {
        fprintf(stderr, "load_sample()> corrupted library\n");
        exit(1);
    }
    s->text = MALLOC(char, t + 1);
    s->path = MALLOC(char, p + 1);
    fread(s->text, 1, t, f);
    fread(s->path, 1, p, f);
    s->text[t] = s->path[p] = '\0';
    s->radius = read_int32(f);
    s->pattern = core_load_pattern(f);
    assert(s->pattern);
    core_promote(s->pattern);
    return 1;
}


Library core_load_library(const char *path)
{
    FILE *f = fopen(path, "rb");
    Library l = MALLOC1(struct LibraryStruct);
    
    if (!f)
    {
        perror(path);
        exit(1);
    }
    
    l->n = 0;//read_int32(f);
    l->allocated = 10;
    l->samples = MALLOC(Sample, l->allocated);
    
    while(1)
    {
        if (!load_sample(append_to_library(l), f))
        {
            l->n--; // last append was false
            break;
        }
        assert(l->samples[l->n - 1].pattern);
    }
    
    fclose(f);
    return l;
}

void core_save_library(Library l, const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;

    if (!f)
    {
        perror(path);
        exit(1);
    }

    //write_int32(l->n, f);

    for (i = 0; i < l->n; i++)
        save_sample(&l->samples[i], f);
    
    fclose(f);
}

void core_destroy_library(Library l)
{
    int i;
    for (i = 0; i < l->n; i++)
    {
        FREE(l->samples[i].text);
        FREE(l->samples[i].path);
        core_destroy_pattern(l->samples[i].pattern);
    }

    if (l->samples)
        FREE(l->samples);
    
    FREE1(l);
}


// ____________________________   recognition   ____________________________

static int samples_conflict(Sample **s, int n)
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


static Opinion *create_opinion(const char *text, ColorCode c)
{
    Opinion *result = MALLOC1(Opinion);
    result->text = text;
    result->color = c;
    return result;
}


static char *fingerprint_recognize(Library l, Pattern p)
{
    char *best = NULL;
    long best_dist = 0x7FFFFFFFL;
    int i;

    for (i = 0; i < l->n; i++)
    {
        int a = l->samples[i].pattern->cc->width  * p->cc->height;
        int b = l->samples[i].pattern->cc->height * p->cc->width;
        if (a > MAX_SIZE_DIFF_COEF * b || b > MAX_SIZE_DIFF_COEF * a)
            continue;

        long dist = fingerprint_distance_squared(l->samples[i].pattern->fingerprint, p->fingerprint);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = l->samples[i].text;
        }
    }

    return best;
}


Opinion *core_recognize_letter(Library l, int no_blacks, unsigned char **pixels, int width, int height)
{
    Pattern p = core_prepare(pixels, width, height);

    // First, match to everything
    Match *matches = MALLOC(Match, l->n);
    Sample **samples = MALLOC(Sample *, l->n);
    int matches_found = 0;
    int i;
    Opinion *opinion;

    assert(l->n > 0);

    for (i = 0; i < l->n; i++)
    {
        assert(l->samples[i].pattern);
        Match m = core_match(l->samples[i].pattern, p);
        if (m)
        {
            matches[matches_found] = m;
            samples[matches_found] = &l->samples[i];
            matches_found++;
        }
    }

    if (!matches_found)
        opinion = create_opinion(NULL, CC_RED);
    else if (!no_blacks && !samples_conflict(samples, matches_found))
        opinion = create_opinion(samples[0]->text, CC_BLACK);
    else
    {
        Match *good_matches = MALLOC(Match, matches_found);
        Sample **good_samples = MALLOC(Sample *, matches_found);
        int good_matches_found = 0;
        int best_match = -1;
        int best_penalty = -1;
        int penalty;
        
        // Another pass over matches, this time with ED-comparison
        for (i = 0; i < matches_found; i++)
        {
            if (core_compare(samples[i]->radius, matches[i], samples[i]->pattern, p, &penalty))
            {
                good_matches[good_matches_found] = matches[i];
                good_samples[good_matches_found] = samples[i];
                good_matches_found++;
            }

            if (i == 0 || penalty < best_penalty)
            {
                best_match = i;
                best_penalty = penalty;
            }
        }

        if (!good_matches_found)
            opinion = create_opinion(samples[best_match]->text, CC_YELLOW);
        else if (samples_conflict(good_samples, good_matches_found))
            opinion = create_opinion(samples[best_match]->text, CC_BLUE);
        else
            opinion = create_opinion(good_samples[0]->text, CC_GREEN);

        FREE(good_matches);
        FREE(good_samples);
    }
    
    if (opinion->color == CC_RED || opinion->color == CC_YELLOW)
    {
        char *alternative = fingerprint_recognize(l, p);

        if (!alternative)
            opinion->color = CC_RED;     
        else if (opinion->color == CC_YELLOW && alternative && !strcmp(alternative, opinion->text))
            opinion->color = CC_MAGENTA;
        
        opinion->text = alternative;
    }

    core_destroy_pattern(p);
    for (i = 0; i < matches_found; i++)
        core_destroy_match(matches[i]);
    
    FREE(samples);    
    FREE(matches);
    return opinion;
}


void core_destroy_opinion(Opinion *op)
{
    FREE1(op);
}
