#include "chaincode.h"
#include "bitmaps.h"
#include "thinning.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>


#define HOT_POINT  2  // a black point is hot when its degree is not 2


Chaincode *chaincode_create(int w, int h)
{
    Chaincode *cc = MALLOC1(Chaincode);
    cc->width = w;
    cc->height = h;
    cc->node_count = 0;
    cc->rope_count = 0;
    cc->node_allocated = 10;
    cc->rope_allocated = 10;
    cc->nodes = MALLOC(Node, cc->node_allocated);
    cc->ropes = MALLOC(Rope, cc->rope_allocated);
    return cc;
}


Rope *chaincode_append_rope(Chaincode *cc)
    LIST_APPEND(Rope, cc->ropes, cc->rope_count, cc->rope_allocated)

Node *chaincode_append_node(Chaincode *cc)
    LIST_APPEND(Node, cc->nodes, cc->node_count, cc->node_allocated)


/* The marking system is tricky. The byte is used to the full :)
 *  _________________________________________________________________________
 * |        |        |         |        |        |         |        |        |
 * | dx = 1 | dx = 0 | dx = -1 | dy = 1 | dy = 0 | dy = -1 |  hot?  | black? |
 * |________|________|_________|________|________|_________|________|________|
 *   1 << 7   1 << 6    1 << 5   1 << 4   1 << 3   1 << 2    1 << 1   1 << 0
 * 
 * Note: hot points are not necessarily marked as black.
 * When we have crossed the (dx, dy) edge of our hot point in either direction,
 * we mark it by setting two bits.
 *
 * The 6th and 3th bits are useless.
 */
#define PASSED_MARK(dx, dy) ((1 << ((dx) + 6)) | (1 << ((dy) + 3)))

    
/* Mark the pixel (x, y) to know that we have passed it along (dx, dy). */
    
static void mark_edge(unsigned char **pixels, int x, int y, int dx, int dy)
{
    pixels[y][x] |= PASSED_MARK(dx, dy);
}


/* Check if we have passed the edge. Complement to mark_edge().
 */
static int passed_edge(unsigned char pixel, int dx, int dy)
{
    /* That means that all bits in PASSED_MARK must be present in pixels[y][x]. */
    return !(~pixel & PASSED_MARK(dx, dy));
}


static int count_passed_edges(unsigned char pixel)
{
    return (passed_edge(pixel, -1,  0) ? 1 : 0)
         + (passed_edge(pixel,  1,  0) ? 1 : 0)
         + (passed_edge(pixel,  0, -1) ? 1 : 0)
         + (passed_edge(pixel,  0,  1) ? 1 : 0);
}


/* (-1,-1) (0,-1) (1,-1)          '7' '8' '9'
 * (-1, 0) (0, 0) (1, 0)    ->    '4' '5' '6'
 * (-1, 1) (0, 1) (1, 1)          '1' '2' '3'
 */
char chaincode_char(int dx, int dy)
{
    return '5' + dx - (dy << 1) - dy; 
}

int chaincode_dx(char c)
{
    return (c - '1') % 3 - 1;
}

int chaincode_dy(char c)
{
    return -((c - '1') / 3 - 1);
}


static char *append_step(char **steps, int *count, int *allocated)
    LIST_APPEND(char, *steps, *count, *allocated)


static int find_node(Chaincode *cc, int x, int y)
{
    int i;
    for (i = 0; i < cc->node_count; i++)
    {
        if (cc->nodes[i].x == x  &&  cc->nodes[i].y == y)
            return i;
    }
    assert(0);
    return -1;
}


/* Assuming we have exactly one neighbor.
 */
static void determine_direction(unsigned char **pixels, int x, int y, int *pdx, int *pdy)
{
    /* These formulas will simply attract us to the neighbor. */
    *pdx = (pixels[y][x + 1] ? 1 : 0) - (pixels[y][x - 1] ? 1 : 0);
    *pdy = (pixels[y + 1][x] ? 1 : 0) - (pixels[y - 1][x] ? 1 : 0);

    assert((pixels[y][x + 1] ? 1 : 0) + (pixels[y][x - 1] ? 1 : 0)
         + (pixels[y + 1][x] ? 1 : 0) + (pixels[y - 1][x] ? 1 : 0) == 1);
}


/* Same, but we might have a black pixel where we came from. */
static void determine_direction_first_time(unsigned char **pixels, int x, int y, int *pdx, int *pdy)
{
    unsigned char save_pixel = pixels[y - *pdy][x - *pdx];
    int new_dx, new_dy;
    pixels[y - *pdy][x - *pdx] = 0; /* reduce the task to previous */
    determine_direction(pixels, x, y, &new_dx, &new_dy);
    pixels[y - *pdy][x - *pdx] = save_pixel;
    *pdx = new_dx;
    *pdy = new_dy;
}

    
/* This is the rope walker.
 * It departs from a given node at a given direction,
 * and travels until the first hot point.
 */
    
static void walk(Chaincode *cc, int start_node, unsigned char **pixels, int dx, int dy)
{
    int x, y;
    int allocated = 20;
    int count = 0;
    Rope *rope;
    char *steps;
    
    assert(0 <= start_node  &&  start_node < cc->node_count);
    
    x = cc->nodes[start_node].x;
    y = cc->nodes[start_node].y;
    
    /* Check that there's indeed a road in the given direction. */
    if (!pixels[y + dy][x + dx])
        return;

    /* Check that we haven't passed here before. */
    if (passed_edge(pixels[y][x], dx, dy))
        return;

    cc->nodes[start_node].rope_indices[count_passed_edges(pixels[y][x])] = cc->rope_count;

    steps = MALLOC(char, allocated);
    mark_edge(pixels, x, y, dx, dy);

    x += dx;
    y += dy;
    *append_step(&steps, &count, &allocated) = chaincode_char(dx, dy);
    if (pixels[y][x] == 1)
    {
        pixels[y][x] = 0;
        determine_direction_first_time(pixels, x, y, &dx, &dy);
        
        while (1)
        {
            x += dx;
            y += dy;
            *append_step(&steps, &count, &allocated) = chaincode_char(dx, dy);
            if (pixels[y][x] != 1)  break;
            pixels[y][x] = 0;

            determine_direction(pixels, x, y, &dx, &dy);
        }
    }

    /* We've arrived at a hot point. Mark the entrance. */
    assert(!passed_edge(pixels[y][x], -dx, -dy));
    mark_edge(pixels, x, y, -dx, -dy);
    
    /* Add the rope. */
    rope = chaincode_append_rope(cc);
    rope->start = start_node;
    rope->end = find_node(cc, x, y);
    rope->steps = REALLOC(char, steps, count);
    rope->length = count;
    cc->nodes[rope->end].rope_indices[count_passed_edges(pixels[y][x]) - 1] = cc->rope_count - 1;
}


/* Search for hot points (with degree != 2).
 * Makes them nodes of the chaincode.
 * Does not mark them in-place, it would be done afterwards
 * (it could interfere with degree counting).
 */
static void search_hot_points(Chaincode *cc, unsigned char **pixels, int w, int h)
{
    int x, y;

    for (y = 0; y < h; y++)
    {
        unsigned char *row = pixels[y];
        
        for (x = 0; x < w; x++) if (row[x])
        {
            int degree = row[x - 1] + row[x + 1] + pixels[y - 1][x] + pixels[y + 1][x];
            if (degree != 2)
            {
                Node *n = chaincode_append_node(cc);
                n->x = x;
                n->y = y;
                n->degree = degree;
                if (degree)
                    n->rope_indices = MALLOC(int, degree);
                else
                    n->rope_indices = NULL;
            }
        }
    }
}


static void mark_hot_points(Chaincode *cc, unsigned char **pixels, int w, int h)
{
    int i;
    for (i = 0; i < cc->node_count; i++)
        pixels[cc->nodes[i].y][cc->nodes[i].x] = HOT_POINT;
}


/* Take a cycle passing through the point (x,y) in this way:
 *
 *    (x,y) <--
 *      |
 *      |
 *      V
 *
 *  There should be such a point in each cycle,
 *  for example, the leftmost of its topmost points.
 */
static void take_cycle(Chaincode *cc, unsigned char **pixels, int x, int y)
{
    Node *n;

    assert(pixels[y + 1][x]);
    assert(pixels[y][x + 1]);

    pixels[y][x] = HOT_POINT;
    
    n = chaincode_append_node(cc);
    n->x = x;
    n->y = y;
    n->degree = 2;
    n->rope_indices = MALLOC(int, 2);
    
    walk(cc, cc->node_count - 1,  pixels, 0,  1);

    /* We should return to the starting point from the left. */
    assert(passed_edge(pixels[y][x], 1, 0));
}

/* By this point, there should be only cycles and hot points remaining.
 * We inject a fake node into each cycle and take them.
 */
static void take_all_cycles(Chaincode *cc, unsigned char **pixels, int w, int h)
{
    int x, y;
    for (y = 0; y < h; y++)
    {
        unsigned char *upper = pixels[y - 1];
        unsigned char *row   = pixels[y];
        
        for (x = 0; x < w; x++) if (row[x] == 1)
        {
            /* we're searching for the following pattern:
             *
             *    0
             *   01 <- not hotpoint
             */
            if (!row[x - 1] && !upper[x])
            {
                take_cycle(cc, pixels, x, y);
            }
        }   
    }
}


Chaincode *chaincode_compute_internal(unsigned char **framework, int w, int h)
{
    int i;
    Chaincode *cc = chaincode_create(w, h);
    search_hot_points(cc, framework, w, h);
    mark_hot_points(cc, framework, w, h);
    
    for (i = 0; i < cc->node_count; i++)
    {
        walk(cc, i, framework,  0, -1);
        walk(cc, i, framework,  0,  1);
        walk(cc, i, framework, -1,  0);
        walk(cc, i, framework,  1,  0);
    }

    take_all_cycles(cc, framework, w, h);

    cc->node_allocated = cc->node_count;
    cc->nodes = REALLOC(Node, cc->nodes, cc->node_allocated);
    cc->rope_allocated = cc->rope_count;
    cc->ropes = REALLOC(Rope, cc->ropes, cc->rope_allocated);
    
    return cc;
}


void chaincode_destroy(Chaincode *cc)
{
    Node *nodes = cc->nodes;
    Rope *ropes = cc->ropes;

    int rope_count = cc->rope_count;
    int node_count = cc->node_count;
    int i;
    
    for (i = 0; i < rope_count; i++)
        if (ropes[i].steps)
            FREE(ropes[i].steps);

    for (i = 0; i < node_count; i++)
    {
        if (nodes[i].rope_indices)
            free(nodes[i].rope_indices);
    }

    if (ropes) FREE(ropes);
    if (nodes) FREE(nodes);
    
    FREE1(cc);
}


void chaincode_print(Chaincode *cc)
{
    int i;
    
    printf("Nodes (%d):\n", cc->node_count);
    for (i = 0; i < cc->node_count; i++)
    {
        int k;
        printf("    %d: (%d, %d) degree %d;", i, cc->nodes[i].x, cc->nodes[i].y, cc->nodes[i].degree);
        for (k = 0; k < cc->nodes[i].degree; k++)
            printf(" %d", cc->nodes[i].rope_indices[k]);
        putchar('\n');        
    }
    
    printf("Ropes (%d):\n", cc->rope_count);

    for (i = 0; i < cc->rope_count; i++)
    {
        int k;
        printf("    %d (%d -> %d) length %d; ", i, cc->ropes[i].start, cc->ropes[i].end, cc->ropes[i].length);
        for (k = 0; k < cc->ropes[i].length; k++)
            putchar(cc->ropes[i].steps[k]);
        putchar('\n');
    }
}


/* Scale a rope by interpreting its scaled steps on a usual grid.
 * Sets new_rope->steps and new_rope->length.
 * `new_rope' may not coincide with `rope', otherwise old `steps' will be lost.
 */
static void scale_rope(Chaincode *cc, Rope *rope, Rope *new_rope, double coef)
{
    double x = cc->nodes[rope->start].x * coef;
    double y = cc->nodes[rope->start].y * coef;
    int n = rope->length;
    char *old_steps = rope->steps;
    int i;
    
    int current_cell_x = (int) x;
    int current_cell_y = (int) y;

    int count = 0;
    int allocated = n;
    char *steps = MALLOC(char, n);
        
    
    for (i = 0; i < n; i++)
    {
        int new_cell_x = current_cell_x;
        int new_cell_y = current_cell_y;
        int dx = 0, dy = 0;
        char step = old_steps[i];
        
        switch(step)
        {
            case '4': x -= coef; dx = -1; new_cell_x = (int) x; break;
            case '6': x += coef; dx =  1; new_cell_x = (int) x; break;
            case '8': y -= coef; dy = -1; new_cell_y = (int) y; break;
            case '2': y += coef; dy =  1; new_cell_y = (int) y; break;
        }

        // Scale one step
        while (new_cell_x != current_cell_x  ||  new_cell_y != current_cell_y)
        {
            current_cell_x += dx;
            current_cell_y += dy;
            *append_step(&steps, &count, &allocated) = step;
        }
    }

    steps = REALLOC(char, steps, count);
    
    new_rope->steps = steps;
    new_rope->length = count;
}


Chaincode *chaincode_scale(Chaincode *cc, double coef)
{
    int i;
    Chaincode *result = MALLOC1(Chaincode);
    result->width  = (int) (cc->width  * coef);
    result->height = (int) (cc->height * coef);
    result->node_count = result->node_allocated = cc->node_count;
    result->rope_count = result->rope_allocated = cc->rope_count;
    result->nodes = MALLOC(Node, result->node_allocated);
    result->ropes = MALLOC(Rope, result->rope_allocated);
    assert(coef > 0);
    assert(coef < 1e5);
    
    /* Copy nodes */
    for (i = 0; i < result->node_count; i++)
    {
        int *rope_indices = MALLOC(int, cc->nodes[i].degree);
        memcpy(rope_indices, cc->nodes[i].rope_indices,
               cc->nodes[i].degree * sizeof(int));
        result->nodes[i].x = (int) (cc->nodes[i].x * coef);
        result->nodes[i].y = (int) (cc->nodes[i].y * coef);
        result->nodes[i].degree = cc->nodes[i].degree;
        result->nodes[i].rope_indices = rope_indices;
    }

    /* Scale ropes */
    for (i = 0; i < result->rope_count; i++)
    {
        result->ropes[i].start = cc->ropes[i].start;
        result->ropes[i].end = cc->ropes[i].end;
        scale_rope(cc, &cc->ropes[i], &result->ropes[i], coef);
    }

    return result;
}


Chaincode *chaincode_compute(unsigned char **pixels, int w, int h)
{
    unsigned char **framework = skeletonize(pixels, w, h, /* 8-conn.: */ 0);
    Chaincode *result = chaincode_compute_internal(framework, w, h);
    free_bitmap_with_margins(framework);
    return result;
}


// ____________________________   render   ___________________________

static void render_path(Chaincode *cc, int rope_index, unsigned char **bitmap, int w, int h)
{
    Rope *rope = &cc->ropes[rope_index];
    Node *node = &cc->nodes[rope->start];
    int x = node->x;
    int y = node->y;
    int i;
    char *s = rope->steps;
    int n = rope->length;
    for (i = 0; i < n; i++)
    {
        x += chaincode_dx(s[i]);
        y += chaincode_dy(s[i]);
        assert(x >= 0 && x < w);
        assert(y >= 0 && y < h); 
        bitmap[y][x] = 1;
    }
    assert(x == cc->nodes[rope->end].x);
    assert(y == cc->nodes[rope->end].y);
}

void chaincode_get_rope_middle_point(Chaincode *cc, int rope_index, int *px, int *py)
{
    Rope *rope = &cc->ropes[rope_index];
    Node *node = &cc->nodes[rope->start];
    int x = node->x;
    int y = node->y;
    int i;
    char *s = rope->steps;
    int n = rope->length;
    for (i = 0; i < n / 2; i++)
    {
        x += chaincode_dx(s[i]);
        y += chaincode_dy(s[i]);
    }
    *px = x;
    *py = y;
}

unsigned char **chaincode_render(Chaincode *cc)
{
    int w = cc->width;
    int h = cc->height;
    int i;
    unsigned char **bitmap = allocate_bitmap(w, h);
    clear_bitmap(bitmap, w, h);
    for (i = 0; i < cc->node_count; i++)
    {
        int x = cc->nodes[i].x;
        int y = cc->nodes[i].y;
        assert(x >= 0 && x < w);
        assert(y >= 0 && y < h); 
        bitmap[y][x] = 1;
    }
    for (i = 0; i < cc->rope_count; i++)
        render_path(cc, i, bitmap, w, h);

    return bitmap;
}

#ifdef TESTING

void get_chaincode_and_render(unsigned char **framework, int w, int h)
{
    unsigned char **copy = allocate_bitmap_with_white_margins(w, h);
    Chaincode *cc;
    unsigned char **rendered;
    assign_bitmap(copy, framework, w, h);
    cc = chaincode_compute_internal(copy, w, h);
    rendered = chaincode_render(cc);
    assert(bitmaps_equal(framework, rendered, w, h));
    free_bitmap(rendered);
    free_bitmap_with_margins(copy);
    chaincode_destroy(cc);
}

void test_render()
{
    int i;
    int w = 3;
    int h = 4;
    unsigned char **bitmap = allocate_bitmap_with_white_margins(w, h);
    for (i = 0; i < (1 << w * h); i++)
    {
        int x, y;
        for (y = 0; y < h; y++) for (x = 0; x < w; x++)
            bitmap[y][x] =  (((1 << (y * w + x)) & i)  ?  1  :  0);
        
        get_chaincode_and_render(bitmap, w, h);
    }
    free_bitmap_with_margins(bitmap);
}

static TestFunction tests[] = {
    test_render,
    NULL
};

TestSuite chaincode_suite = {"chaincode", NULL, NULL, tests};

#endif
