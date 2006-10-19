#include "common.h"
#include "wordcut.h"
#include "chaincode.h"
#include "thinning.h"
#include "bitmaps.h"
#include "rle.h"

#include <string.h>


static void get_rope_projection(unsigned char *result, Chaincode *cc, int rope_index)
{
    Rope *r = &cc->ropes[rope_index];
    int x = cc->nodes[r->start].x;
    int n = r->length;
    char *s = r->steps;
    int i;
    int left_margin = 0, right_margin = 0;

    if (cc->nodes[r->start].degree == 1)
        left_margin = 1;
    if (cc->nodes[r->end].degree == 1)
        right_margin = 1;
    
    memset(result, 0, cc->width);
    
    
    for (i = 0; i < left_margin - 1; i++)
        x += chaincode_dx(s[i]);
    result[x] = 1;
    if (!left_margin)
        left_margin = 1;

    for (i = left_margin - 1; i < n - right_margin; i++)
    {
        int dx = chaincode_dx(s[i]);
        if (dx)
        {
            x += dx;
            result[x]++;
        }
    }
}


static int other_end(int node_index, Rope *rope)
{
    if (rope->start == node_index)
        return rope->end;
    else
        return rope->start;
}


/* A hedgehog is a vertex whose adjacent vertices (except that via rope #rope_index)
 * are all hanging (have degree 1).
 */

static int is_hedgehog(Chaincode *cc, int node_index, int rope_index)
{
    int i;
    Node *n = &cc->nodes[node_index];
    for (i = 0; i < n->degree; i++)
    {
        int current_rope = n->rope_indices[i];
        if (current_rope == rope_index)
            continue;
        if (cc->nodes[other_end(node_index, &cc->ropes[current_rope])].degree != 1)
            return 0;
    }
    return 1;
}


static int get_shield_level(Chaincode *cc, int rope_index)
{
    Rope *r = &cc->ropes[rope_index];
    Node *s = &cc->nodes[r->start];
    Node *e = &cc->nodes[r->end];
    if (is_hedgehog(cc, r->start, rope_index) || is_hedgehog(cc, r->end, rope_index))
        return 1;

    if (2 * abs(s->y - e->y) > abs(s->x - e->x))
        return 2;

    return 255;
}


static void fill_shields(int w, unsigned char *shields, int level, unsigned char *projection)
{
    int i;
    for (i = 0; i < w; i++)
    {
        if (projection[i] > 1)
            shields[i] = 0;
        else if (projection[i])
        {
            if (shields[i] > level)
                shields[i] = level;
        }
    }
}


static int *make_histogram(unsigned char **pixels, int w, int h)
{
    int *histogram = MALLOC(int, w);
    int i, j;
    memset(histogram, 0, w * sizeof(int));
    for (i = 0; i < h; i++)
        for (j = 0; j < w; j++)
            if (pixels[i][j])
                histogram[j]++;
    return histogram;
}


static int score_cut(int *histogram, int mid, int cut)
{
    return histogram[cut] * histogram[cut] + (cut - mid) * (cut - mid);
}


static int cut_into_window(int *histogram, int begin, int end)
{
    int mid = (begin + end) / 2;
    int i;
    int best = begin;
    int best_score = score_cut(histogram, mid, begin);

    for (i = begin + 1; i < end; i++)
    {
        int score = score_cut(histogram, mid, i);
        if (score < best_score)
        {
            best = i;
            best_score = score;
        }
    }

    return best;
}


WordCut *cut_word(unsigned char **pixels, int w, int h)
{
    unsigned char *projection = MALLOC(unsigned char, w);
    int *histogram = make_histogram(pixels, w, h);
    unsigned char *shields = MALLOC(unsigned char, w);
    WordCut *wc = MALLOC1(WordCut);
    
    Chaincode *cc;
    int i;
    int rope_count;
    
    cc = chaincode_compute(pixels, w, h);
    rope_count = cc->rope_count;

    /* shield level n prevents cuts with level above n.
     * (the less the shield level, the stronger it is)
     */
    wc->count = 0;
    wc->position = MALLOC(int, w);
    wc->window_start = MALLOC(int, w);
    wc->window_end = MALLOC(int, w);
    wc->level = MALLOC(unsigned char, w);
    wc->max_level = 1; /* XXX */
    
    memset(shields, 255, w); /* weakest shields, prevent no cuts */

    for (i = 0; i < rope_count; i++)
    {
        int shield_level = get_shield_level(cc, i);
        get_rope_projection(projection, cc, i);
        fill_shields(w, shields, shield_level, projection);
    }

    for (i = 0; i < w && shields[i] == 255; i++) {}
    
    while (i < w)
    {
        int begin, end;
        
        /* skip shields */
        while (i < w && shields[i] < 255) i++;
        begin = i;
        /* skip window */
        while (i < w && shields[i] == 255) i++;
        end = i;
        if (i == w) break;

        /* add the cut */
        wc->level[wc->count] = 1;
        wc->window_start[wc->count] = begin;
        wc->window_end[wc->count] = end;
        wc->position[wc->count] = cut_into_window(histogram, begin, end);
        wc->count++;
    }

    wc->level        = REALLOC(unsigned char, wc->level,        wc->count); 
    wc->position     = REALLOC(int,           wc->position,     wc->count); 
    wc->window_start = REALLOC(int,           wc->window_start, wc->count); 
    wc->window_end   = REALLOC(int,           wc->window_end,   wc->count); 
    
    FREE(projection);
    FREE(shields);
    FREE(histogram);
    chaincode_destroy(cc);
    return wc;
}


void destroy_word_cut(WordCut *w)
{
    FREE(w->level);
    FREE(w->position);
    FREE(w->window_start);
    FREE(w->window_end);
    FREE1(w);
}
