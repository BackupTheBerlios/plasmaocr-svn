#include "chaincode.h"
#include "chaincode.h"
#include <assert.h>


#define HOT_POINT  2


Rope *chaincode_append_rope(Chaincode *cc)
    LIST_APPEND(Rope, cc->ropes, cc->rope_count, cc->rope_allocated)
    
Node *chaincode_append_node(Chaincode *cc)
    LIST_APPEND(Node, cc->nodes, cc->node_count, cc->node_allocated)


/* Mark the pixel (x, y) to know that we have passed it along (dx, dy).
 * The marking system is tricky. The byte is used to the full :)
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
 * The 6th and 4th bits are useless.
 */
static void mark_edge(unsigned char **pixels, int x, int y, int dx, int dy)
{
    pixels[y][x] |= (1 << (dx + 6)) | (1 << (dy + 3));
}


/* Check if we have passed the edge. Complement to mark_edge().
 */
static int passed_edge(unsigned char **pixels, int x, int y, int dx, int dy)
{
    return pixels[y][x] & (1 << (dx + 6)) | (1 << (dy + 3));
}


/* (-1,-1) (0,-1) (1,-1)          '7' '8' '9'
 * (-1, 0) (0, 0) (1, 0)    ->    '4' '5' '6'
 * (-1, 1) (0, 1) (1, 1)          '1' '2' '3'
 */
char chaincode_char(int dx, int dy)
{
    return '5' + dx + (dy << 1) + dy; 
}


static void append_step(char **rope, int *count, int *allocated)
    LIST_APPEND(char, *rope, *count, *allocated)


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

    
/* This is the rope walker.
 * It departs from a given node at a given direction,
 * and travels until the first hot point.
 */
    
static void walk(Chaincode *cc, int start_node, unsigned char **pixels, int dx, int dy)
{
    int x;
    int nx, ny;
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
    if (passed_edge(pixels, x, y, dx, dy))
        return;
    
    steps = MALLOC(char, allocated);    
    mark_edge(pixels, x, y, dx, dy);

    while (1)
    {
        x += dx;
        y += dy;
        *append_step(&steps, &count, &allocated) = chaincode_char(dx, dy);

        if (pixels[y][x] != 1)
            break;
        
        pixels[y][x] = 0;

        /* Where to go now? We have exactly one neighbour,
         * so in the following formulas one term will be 1,
         * others will be zero.
         */
        dx = (pixels[y][x + 1] ? 1 : 0) - (pixels[y][x - 1] ? 1 : 0);
        dy = (pixels[y + 1][x] ? 1 : 0) - (pixels[y - 1][x] ? 1 : 0);
    }

    /* We've arrived at a hot point. Mark the entrance. */
    assert(!passed_edge(pixels, x, y, -dx, -dy));
    mark_edge(pixels, x, y, -dx, -dy);
    
    /* Add the rope. */
    rope = chaincode_append_rope(cc);
    rope->start = start_node;
    rope->end = find_node(cc, x, y);
    rope->steps = REALLOC(char, steps, count);;
    rope->length = count;
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
                Node *n = chaincode_add_node(cc);
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
    for (i = 0; i < cc->nodes_count; i++)
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
    
    n = chaincode_add_node(cc);
    n->x = x;
    n->y = y;
    n->degree = 2;
    n->rope_indices = MALLOC(int, 2);
    
    walk(cc, cc->node_count - 1,  0, -1);
    walk(cc, cc->node_count - 1,  0,  1);
    walk(cc, cc->node_count - 1, -1,  0);
    walk(cc, cc->node_count - 1,  1,  0);
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
             *   00
             *   01 <- not hotpoint
             */
            if (!row[x - 1] && !upper[x - 1] && !upper[x])
            {
                take_cycle(cc, pixels, x, y);
            }
        }   
    }
}


Chaincode *chaincode_by_framework(unsigned char **framework, int w, int h)
{
    int i;
    Chaincode *cc = chaincode_create();
    search_hot_points(cc, framework, w, h);
    mark_hot_points(cc, framework, w, h);
    for (i = 0; i < cc->node_count; i++)
    {
        walk(cc, i,  0, -1);
        walk(cc, i,  0 , 1);
        walk(cc, i, -1,  0);
        walk(cc, i,  1,  0);
    }
    take_all_cycles(cc, framework, w, h);
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
        free(ropes[i]->steps);

    for (i = 0; i < node_count; i++)
    {
        if (nodes[i].degree)
            free(nodes[i]->rope_indices);
    }

    free(ropes);
    free(nodes);
    free(cc);
}
