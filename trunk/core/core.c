#include "common.h"
#include "chaincode.h"
#include "editdist.h"
#include "core.h"
#include <assert.h>


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


Pattern core_prepare(unsigned char **pixels, int w, int h)
{
    Pattern p = MALLOC1(struct PatternStruct);
    Chaincode *cc = chaincode_compute(pixels, w, h);
    double coef = ((double) COMMON_HALF_PERIMETER) / (w + h);
    p->cc = chaincode_scale(cc, coef);
    chaincode_destroy(cc);
    copy_node_coordinates(p);
    compute_rope_medians(p);
    p->ropes_backwards = NULL;
    return p;
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


int core_compare(int radius, Match m, Pattern p1, Pattern p2)
{
    int r = p1->cc->rope_count;
    int i;
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
    
    // Now pass all the ropes, applying editing distance
    for (i = 0; i < r; i++)
    {
        Rope *r1 = &p1->cc->ropes[i];
        Rope *r2 = &p2->cc->ropes[rope_mapping[i]];
        int s1 = node_mapping[r1->start];
        int e1 = node_mapping[r1->end];
        int s2 = r2->start;
        int e2 = r2->end;

        if (s1 == s2 && e1 == e2
            && edit_distance(radius, r1->steps, r1->length, r2->steps, r2->length) <= 0)
            continue;
        
        if (s1 == e2 && e1 == s2
            && edit_distance(radius, back[i], r1->length, r2->steps, r2->length) <= 0)
            continue;

        return 0;
    }

    return 1;
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
