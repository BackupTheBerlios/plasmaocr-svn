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


#define MAX_SIZE_DIFF_COEF 1.3
#define COMMON_HALF_PERIMETER 32


struct MatchStruct
{
    int *node_mapping;
    int *rope_mapping;
    int swap;
};


struct PatternStruct
{
    Chaincode *cc;
    int *nodes_x;
    int *nodes_y;
    int *rope_medians_x;
    int *rope_medians_y;
    char **ropes_backwards;
    Fingerprint fingerprint;
};


static void copy_node_coordinates(Pattern p)
{
    int n = p->cc->node_count;
    Node *nodes = p->cc->nodes;
    int *x = MALLOC(int, n);
    int *y = MALLOC(int, n);
    int i;

    for (i = 0; i < n; i++)
    {
        x[i] = nodes[i].x;
        y[i] = nodes[i].y;
    }

    p->nodes_x = x;
    p->nodes_y = y;
}
     

static void compute_rope_medians(Pattern p)
{
    int r = p->cc->rope_count;
    int *x = MALLOC(int, r);
    int *y = MALLOC(int, r);
    int i;
    
    for (i = 0; i < r; i++)
        chaincode_get_rope_middle_point(p->cc, i, &x[i], &y[i]);
    
    p->rope_medians_x = x;
    p->rope_medians_y = y;
}


static Pattern chaincode_to_pattern(Chaincode *cc)
{
    Pattern p = MALLOC1(struct PatternStruct);
    p->cc = cc;    
    copy_node_coordinates(p);
    compute_rope_medians(p);
    p->ropes_backwards = NULL;
    return p;
}


Pattern core_prepare(unsigned char **pixels, int w, int h)
{
    Chaincode *cc = chaincode_compute(pixels, w, h);
    double coef = ((double) COMMON_HALF_PERIMETER) / (w + h);
    Pattern p = chaincode_to_pattern(chaincode_scale(cc, coef));
    chaincode_destroy(cc);
    get_fingerprint_bw(pixels, w, h, &p->fingerprint);
    return p;
}

Pattern core_load_pattern(FILE *f)
{
    Pattern p = chaincode_to_pattern(chaincode_load(f));
    fread(&p->fingerprint, 1, sizeof(Fingerprint), f);
    return p;
}


void core_save_pattern(Pattern p, FILE *f)
{
    chaincode_save(p->cc, f);
    fwrite(&p->fingerprint, 1, sizeof(Fingerprint), f);    
}

char *reverse_rope(Rope *rope)
{
    int i;
    char *result;
    char *steps = rope->steps;
    int n = rope->length;
    if (!n)
        return NULL;

    result = MALLOC(char, n);
    for (i = 0; i < n; i++)
        result[i] = '1' + '9' - steps[n - i - 1];

    return result;
}


void core_promote(Pattern p)
{
    int i;
    int r = p->cc->rope_count;
    Rope *ropes = p->cc->ropes;
    
    if (!p) return;
    if (p->ropes_backwards) return;

    // reverse all ropes
    p->ropes_backwards = MALLOC(char *, r);
    for (i = 0; i < r; i++)
        p->ropes_backwards[i] = reverse_rope(&ropes[i]);
}


// Returns the index of the nearest point to (x,y).
// n > 0
// Bug: doesn't work well with big numbers.
static int nearest_point(int n, int *xs, int *ys, int x, int y)
{
    int best = -1;
    unsigned best_dist = (unsigned) (-1);
    int i;
    
    for (i = 0; i < n; i++)
    {
        int dx = xs[i] - x;
        int dy = ys[i] - y;
        unsigned dist = dx * dx + dy * dy;
        if (dist < best_dist)
        {
            best = i;
            best_dist = dist;
        }
    }
    
    return best;
}


// Match two clouds of points.
static int *match_clouds(int n, int *x1, int *y1, int *x2, int *y2)
{
    int i, j;
    int *result;

    if (n == 0)
        return NULL;

    result = MALLOC(int, n);

    for (i = 0; i < n; i++)
    {
        int best = nearest_point(n, x2, y2, x1[i], y1[i]);
        
        // check that the match is unique
        for (j = 0; j < i; j++)
        {
            if (result[j] == best)
            {
                FREE(result);
                return NULL;
            }
        }

        result[i] = best;
    }
    
    return result;
}


Match core_match(Pattern p1, Pattern p2)
{
    int swap = 0;
    int *node_mapping;
    int *rope_mapping;
    Match result;
    int n = p1->cc->node_count;
    int r = p1->cc->rope_count;
    
    if (n != p2->cc->node_count)
        return NULL;
    if (r != p2->cc->rope_count)
        return NULL;
    
    assert(p1->ropes_backwards || p2->ropes_backwards);
    if (!p1->ropes_backwards)
    {
        swap = 1;
        Pattern tmp = p1;
        p1 = p2;
        p2 = tmp;        
    }
    
    node_mapping = match_clouds(n, p1->nodes_x, p1->nodes_y,
                                   p2->nodes_x, p2->nodes_y);
    if (n && !node_mapping)
        return NULL;
    
    rope_mapping = match_clouds(r, p1->rope_medians_x, p1->rope_medians_y,
                                   p2->rope_medians_x, p2->rope_medians_y);
    if (r && !rope_mapping)
    {
        FREE(node_mapping);
        return NULL;
    }

    result = MALLOC1(struct MatchStruct);
    result->node_mapping = node_mapping;
    result->rope_mapping = rope_mapping;
    result->swap = swap;
    return result;
}


int core_compare(int radius, Match m, Pattern p1, Pattern p2, int *penalty)
{
    int r = p1->cc->rope_count;
    int i;
    int result = 1;
    int *node_mapping;
    int *rope_mapping;
    char **back;
    
    if (!m) return 0;
    assert(r == p2->cc->rope_count);

    node_mapping = m->node_mapping;
    rope_mapping = m->rope_mapping;

    if (m->swap) // p1 should be promoted
    {
        Pattern tmp = p1;
        p1 = p2;
        p2 = tmp;
    }
    
    back = p1->ropes_backwards;

    if (penalty) *penalty = 0;
    
    // Now pass all the ropes, applying editing distance
    for (i = 0; i < r; i++)
    {
        Rope *r1 = &p1->cc->ropes[i];
        Rope *r2 = &p2->cc->ropes[rope_mapping[i]];
        int s1 = node_mapping[r1->start];
        int e1 = node_mapping[r1->end];
        int s2 = r2->start;
        int e2 = r2->end;
        int ed;
        
        if (s1 == s2 && e1 == e2)
            ed = edit_distance(radius, r1->steps, r1->length, r2->steps, r2->length);
        else if (s1 == e2 && e1 == s2)            
            ed = edit_distance(radius, back[i], r1->length, r2->steps, r2->length);
        else
            return 0;
        
        if (ed > 0)
            result = 0;
        
        if (!result && !penalty) return 0;
        *penalty += ed; 
    }

    return result;
}


void core_destroy_match(Match m)
{
    if (!m) return;
    if (m->node_mapping) FREE(m->node_mapping);
    if (m->rope_mapping) FREE(m->rope_mapping);
    FREE(m);
}


static void unpromote(Pattern p)
{
    char **back = p->ropes_backwards;
    int r = p->cc->rope_count;
    int i;

    for (i = 0; i < r; i++)
    {
        if (back[i])
            FREE(back[i]);
    }

    FREE(back);
    p->ropes_backwards = NULL;
}


void core_destroy_pattern(Pattern p)
{
    if (p->ropes_backwards)
        unpromote(p);
    
    chaincode_destroy(p->cc);
    if (p->nodes_x) FREE(p->nodes_x);
    if (p->nodes_y) FREE(p->nodes_y);
    if (p->rope_medians_x) FREE(p->rope_medians_x);
    if (p->rope_medians_y) FREE(p->rope_medians_y);
    FREE1(p);
}

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
