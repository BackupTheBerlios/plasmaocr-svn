#include "common.h"
#include "pattern.h"
#include "chaincode.h"
#include "shiftcut.h"
#include "thinning.h"
#include "bitmaps.h"
#include "editdist.h"
#include "io.h"
#include "pnm.h"
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
    float *nodes_x;
    float *nodes_y;
    float *rope_medians_x;
    float *rope_medians_y;
    char **ropes_backwards;
    Fingerprint fingerprint;
};


struct PatternCacheStruct
{
    unsigned char **framework;
};


static void copy_node_coordinates(Pattern p)
{
    int n = p->cc->node_count;
    Node *nodes = p->cc->nodes;
    float *x = MALLOC(float, n);
    float *y = MALLOC(float, n);
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
    float *x = MALLOC(float, r);
    float *y = MALLOC(float, r);
    int i;
    
    for (i = 0; i < r; i++)
        chaincode_get_rope_middle_point(p->cc, i, &x[i], &y[i]);
    
    p->rope_medians_x = x;
    p->rope_medians_y = y;
}


static void chaincode_into_pattern(Pattern p, Chaincode *cc)
{
    p->cc = cc;    
    copy_node_coordinates(p);
    p->ropes_backwards = NULL;
}


static Pattern chaincode_to_pattern_scaled(Chaincode *cc)
{
    Pattern p = MALLOC1(struct PatternStruct);
    double coef = ((double) COMMON_HALF_PERIMETER) / (cc->width + cc->height);
    int i;
    p->cc = cc;
    compute_rope_medians(p);
    chaincode_into_pattern(p, chaincode_scale(cc, coef));
    for (i = 0; i < cc->rope_count; i++)
    {
        p->rope_medians_x[i] *= coef;
        p->rope_medians_y[i] *= coef;
    }
    return p;
}


Pattern create_pattern(unsigned char **pixels, int width, int height)
{
    unsigned char **window;
    int w, h;
    int free_window = get_bbox_window(pixels, width, height, &window, &w, &h); 
    
    Chaincode *cc = chaincode_compute(window, w, h);
    Pattern p = chaincode_to_pattern_scaled(cc);
    chaincode_destroy(cc);
    get_fingerprint_bw(pixels, w, h, &p->fingerprint);
    
    if (free_window)
        FREE(window);

    return p;
}


PatternCache create_pattern_cache(unsigned char **pixels, int width, int height)
{
    PatternCache result = MALLOC1(struct PatternCacheStruct);
    result->framework = skeletonize(pixels, width, height, /* 8-conn.: */ 0);
    return result;
}

void destroy_pattern_cache(PatternCache p)
{
    free_bitmap_with_margins(p->framework);
    FREE1(p);
}


Pattern create_pattern_from_cache(unsigned char **pixels, int width, int height,
                                  int left, int top, int p_w, int p_h,
                                  PatternCache pc)
{
    unsigned char **buffer;
    Chaincode *cc;
    int i;
    Pattern p;
    
    assert(p_w > 0);
    assert(p_h > 0);
    assert(left >= 0);
    assert(top >= 0);
    assert(left + p_w <= width);
    assert(top  + p_h <= height);

    tighten_to_bbox(pixels, width, &left, &top, &p_w, &p_h);
    buffer = allocate_bitmap_with_white_margins(p_w, p_h);

    assign_bitmap_with_offsets(buffer, pc->framework + top, p_w, p_h, 0, left);
    cc = chaincode_compute_internal(buffer, p_w, p_h);
    free_bitmap_with_margins(buffer);
    p = chaincode_to_pattern_scaled(cc);
    chaincode_destroy(cc);
    
    buffer = MALLOC(unsigned char *, p_h);
    for (i = 0; i < p_h; i++)
        buffer[i] = pixels[i + top] + left;
    get_fingerprint_bw(buffer, p_w, p_h, &p->fingerprint);
    FREE(buffer);
    return p;
}


Pattern load_pattern(FILE *f)
{
    Pattern p = MALLOC1(struct PatternStruct);
    chaincode_into_pattern(p, chaincode_load(f));
    p->rope_medians_x = MALLOC(float, p->cc->rope_count);
    p->rope_medians_y = MALLOC(float, p->cc->rope_count);
    fread(p->rope_medians_x, p->cc->rope_count, sizeof(float), f);
    fread(p->rope_medians_y, p->cc->rope_count, sizeof(float), f);
    fread(&p->fingerprint, 1, sizeof(Fingerprint), f);
    return p;
}


void save_pattern(Pattern p, FILE *f)
{
    chaincode_save(p->cc, f);
    fwrite(p->rope_medians_x, p->cc->rope_count, sizeof(float), f);
    fwrite(p->rope_medians_y, p->cc->rope_count, sizeof(float), f);
    fwrite(&p->fingerprint, 1, sizeof(Fingerprint), f);
}


static char *reverse_rope(Rope *rope)
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


void promote_pattern(Pattern p)
{
    int i;
    int r = p->cc->rope_count;
    Rope *ropes = p->cc->ropes;
    
    if (!p) return;
    if (p->ropes_backwards) return;

    /* reverse all ropes */
    p->ropes_backwards = MALLOC(char *, r);
    for (i = 0; i < r; i++)
        p->ropes_backwards[i] = reverse_rope(&ropes[i]);
}


/* Returns the index of the nearest point to (x,y).
 * n > 0
 * Bug: doesn't work well with big numbers.
 */
static int nearest_point(int n, float *xs, float *ys, float x, float y)
{
    int best = -1;
    double best_dist = 1e10;
    int i;
    
    for (i = 0; i < n; i++)
    {
        float dx = xs[i] - x;
        float dy = ys[i] - y;
        double dist = dx * dx + dy * dy;
        if (dist < best_dist)
        {
            best = i;
            best_dist = dist;
        }
    }
    
    return best;
}


/* Match two clouds of points. */
static int *match_clouds(int n, float *x1, float *y1, float *x2, float *y2)
{
    int i, j;
    int *result;

    if (n == 0)
        return NULL;

    result = MALLOC(int, n);

    for (i = 0; i < n; i++)
    {
        int best = nearest_point(n, x2, y2, x1[i], y1[i]);
        
        /* check that the match is unique */
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


Match match_patterns(Pattern p1, Pattern p2)
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
        Pattern tmp = p1;
        swap = 1;
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


int compare_patterns(int radius, Match m, Pattern p1, Pattern p2, int *penalty)
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

    if (m->swap) /* p1 should be promoted */
    {
        Pattern tmp = p1;
        p1 = p2;
        p2 = tmp;
    }
    
    back = p1->ropes_backwards;

    if (penalty) *penalty = 0;
    
    /* Now pass all the ropes, applying editing distance */
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


void destroy_match(Match m)
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


void free_pattern(Pattern p)
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

int pattern_size_test(Pattern p1, Pattern p2)
{
    int a = p1->cc->width  * p2->cc->height;
    int b = p2->cc->height * p1->cc->width;
    return !(a > MAX_SIZE_DIFF_COEF * b || b > MAX_SIZE_DIFF_COEF * a);
}

long patterns_shiftcut_dist(Pattern p1, Pattern p2)
{
    if (!pattern_size_test(p1, p2))
        return 0x7FFFFFFFL;

    return fingerprint_distance_squared(p1->fingerprint, p2->fingerprint);
}

/* ______________________________   testing   __________________________________ */

#ifdef TESTING

void assert_patterns_equal(Pattern p1, Pattern p2)
{
    int n, r;
    assert_chaincodes_equal(p1->cc, p2->cc);
    n = p1->cc->node_count;
    r = p1->cc->rope_count;

    assert(!memcmp(p1->nodes_x, p2->nodes_x, n * sizeof(float)));
    assert(!memcmp(p1->nodes_y, p2->nodes_y, n * sizeof(float)));
    assert(!memcmp(p1->rope_medians_x, p2->rope_medians_x, r * sizeof(float)));
    assert(!memcmp(p1->rope_medians_y, p2->rope_medians_y, r * sizeof(float)));
    assert(!memcmp(p1->fingerprint, p2->fingerprint, sizeof(Fingerprint)));
}

/*static void assert_match_reflexivity(Pattern p)
{
    Match m;
    promote_pattern(p);
    m = match_patterns(p, p);
    assert(m);
    destroy_match(m);
}*/

static void test_save_load_on_pbm(FilePair fp, const char *path)
{
    unsigned char **pixels;
    FILE *f;
    Pattern p, p2;
    int w;
    int h;
    load_pnm(path, &pixels, &w, &h);
    p = create_pattern(pixels, w, h);
    //assert_match_reflexivity(p);
    f = file_pair_write(fp);
    save_pattern(p, f);
    f = file_pair_read(fp);
    p2 = load_pattern(f);
    assert_patterns_equal(p, p2);
    free_pattern(p);
    free_pattern(p2);
    free_bitmap(pixels);    
}

static void test_save_load(void)
{
    FilePair fp = file_pair_open();
    test_save_load_on_pbm(fp, "test/i.pbm");
    file_pair_close(fp);
}

TestFunction tests[] = {
    test_save_load, 
    NULL
};

TestSuite pattern_suite = {"patterns", NULL, NULL, tests};


#endif
