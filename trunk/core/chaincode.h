#ifndef PLASMA_OCR_CHAINCODE_H
#define PLASMA_OCR_CHAINCODE_H


#include "common.h"
#include <stdio.h>

typedef struct
{
    int x, y;
    int degree;        /* may be 2 (on cycles, a node is injected somewhere) */
    int *rope_indices; /* the same rope may be here twice */
} Node;


/* Each step direction is encoded into a single digit:
 *   '8' - up
 *   '2' - down
 * etc. (just look at the numeric keypad)
 * Diagonal steps are currently not used.
 */

typedef struct
{
    int length;     /* in steps */
    char *steps;
    int start, end; /* node indices */
} Rope;


typedef struct
{
    Node *nodes;
    Rope *ropes;
    int node_count;
    int rope_count;
    int width;           /* of the original picture */
    int height;
    int node_allocated;  /* in fact, these two fields should be hidden */
    int rope_allocated;
} Chaincode;


FUNCTIONS_BEGIN

/* Append a new node/rope to the end.
 * Doesn't initialize them.
 */
Rope *chaincode_append_rope(Chaincode *);
Node *chaincode_append_node(Chaincode *);


/* Compute the chaincode from a framework.
 * A framework is obtained by thinning, see `thinning.h'.
 * 
 * `framework':
 *      should contain only 0/1
 *      should have white 1-pixel margins
 *      should be extracted with 4-connectivity
 *      (arrays returned by framework_compute() are fine)
 *      IS OVERWRITTEN WITH NODE MAP (only nodes will be nonzero)
 */
Chaincode *chaincode_compute_internal(unsigned char **framework, int w, int h);


/* Compute the chaincode.
 * This is equivalent to skeletonize() + chaincode_compute_internal().
 */
Chaincode *chaincode_compute(unsigned char **pixels, int w, int h);


void chaincode_print(Chaincode *);


Chaincode *chaincode_create(int width, int height);
void chaincode_destroy(Chaincode *);


/* Scale the chaincode (creating a copy).
 * This is a cheap alternative to vectorization.
 * It's always better to scale down (coef < 1).
 */
Chaincode *chaincode_scale(Chaincode *cc, double coef);

unsigned char **chaincode_render(Chaincode *cc);

void chaincode_get_rope_middle_point(Chaincode *cc, int rope_index, int *px, int *py);

Chaincode *chaincode_load(FILE *f);
void chaincode_save(Chaincode *cc, FILE *f);

char chaincode_char(int dx, int dy);
int chaincode_dx(char c);
int chaincode_dy(char c);

FUNCTIONS_END

#ifdef TESTING
extern TestSuite chaincode_suite;
#endif


#endif
