#ifndef LAYOUT_H
#define LAYOUT_H


#include <stdio.h>


typedef struct
{
    int x, y;
    int w, h;
    int is_space;
} Box;


typedef struct
{
    Box *boxes;
    int count;
    int allocated;
} BoxList;


typedef enum
{
    LIST_LEAF, /* then `items' is a pointer to a Box, not to a list. That's dirty. */
    LIST_LINE,
    LIST_BLOCK,
    LIST_PAGE
} ListType;


typedef struct List
{
    ListType type;
    union
    {
        struct
        {
            struct List *items;
            int count;
        };
        struct
        {
            Box *box;
            int index;
        };
    };
} List;


/* Init the list structure, allocating `count' items unless type is list_leaf.
 */
void list_init(List *, ListType type, int count);

BoxList *box_list_create(void);
void box_list_finalize(BoxList *l);
void box_list_destroy(BoxList *l);
Box *box_allocate(BoxList *boxlist);
BoxList *parse_ocrad(List *, const char *path);

/* Set `box' pointers in leaves.
 * NOTE: don't call this before the boxlist is finalized,
 * since it may be moved by realloc.
 */
void list_resolve(List *l, Box *boxes);

#endif
