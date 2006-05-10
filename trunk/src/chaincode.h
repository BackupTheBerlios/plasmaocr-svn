#ifndef PLASMA_OCR_CHAINCODE_H
#define PLASMA_OCR_CHAINCODE_H


typedef struct
{
    int x, y;
    int degree; // may be 2 (on cycles, a node is injected somewhere)
    int *rope_indices; // the same rope may be here twice
} Node;


/* Each step direction is encoded into a single digit:
 *   '8' - up
 *   '2' - down
 * etc. (just look at the numeric keypad)
 * Diagonal steps are currently not used.
 */

typedef struct
{
    int length;     // in steps
    char *steps;
    int start, end; // node indices
} Rope;


typedef struct
{
    Node *nodes;
    Rope *ropes;
    int node_count;
    int rope_count;
    int node_allocated;
    int rope_allocated;
} Chaincode;


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
 *      (arrays returned by framework_compute() are fine)
 *      IS OVERWRITTEN WITH NODE MAP (only nodes will be nonzero)
 *      do not use framework_force_8_connectivity() on it!
 */
Chaincode *chaincode_compute(unsigned char **framework, int w, int h);


void chaincode_print(Chaincode *);


Chaincode *chaincode_create(void);
void chaincode_destroy(Chaincode *);


#endif
