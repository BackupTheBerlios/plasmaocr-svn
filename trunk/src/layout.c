#include "layout.h"
#include "memory.h"
#include "linewise.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>


BoxList *box_list_create(void)
{
    BoxList *l = (BoxList *) malloc(sizeof(BoxList));
    l->count = 0;
    l->allocated = 256;
    l->boxes = (Box *) malloc(l->allocated * sizeof(Box));
    return l;
}


void box_list_finalize(BoxList *l)
{
    l->allocated = l->count;
    l->boxes = (Box *) realloc(l->boxes, l->allocated * sizeof(Box));
}


void box_list_destroy(BoxList *l)
{
    free(l->boxes);
    free(l);
}

Box *box_allocate(BoxList *boxlist)
    LIST_APPEND(Box, boxlist->boxes, boxlist->count, boxlist->allocated)

void list_init(List *l, ListType type, int count)
{
    l->type = type;
    l->count = count;
    if (type != LIST_LEAF && count)
        l->items = (List *) malloc(count * sizeof(List));
    else
        l->items = NULL;
}

void list_resolve(List *l, Box *boxes)
{
    if (l->type == LIST_LEAF)
        l->box = &boxes[l->index];
    else
    {
        int n = l->count;
        int i;
        
        for (i = 0; i < n; i++)
            list_resolve(&l->items[i], boxes);
    }
}


const char ocrad_block_counter[] = "total text blocks ";
int        ocrad_block_counter_length =
    sizeof(ocrad_block_counter) - 1;

const char ocrad_char_counter[] = "chars ";
int        ocrad_char_counter_length =
    sizeof(ocrad_char_counter) - 1;

const char ocrad_line_counter[] = "lines ";
int        ocrad_line_counter_length =
    sizeof(ocrad_line_counter) - 1;


static void parse_box(BoxList *boxlist, List *l, LinewiseReader r)
{
    Box *b = box_allocate(boxlist);
    const char *s = linewise_read(r);
    const char *first_quote;
    assert(s);
    sscanf(s, "%d %d %d %d", &b->x, &b->y, &b->w, &b->h);
    first_quote = strchr(s, '\'');
    assert(first_quote);
    b->is_space  =  first_quote[1] == ' ';
    list_init(l, LIST_LEAF, 1);
    l->index = boxlist->count - 1;
}


static void parse_line(BoxList *boxlist, List *l, LinewiseReader r)
{
    const char *s = linewise_read(r);
    
    // read a line like this:
    //      line 5 chars 1 height 3
    // the only interesting figure here is the number of chars
    char *nchars = strstr(s, ocrad_char_counter) + ocrad_char_counter_length;
    int n = atoi(nchars);
    int i;
    
    list_init(l, LIST_LINE, n);
    for (i = 0; i < n; i++)
        parse_box(boxlist, &l->items[i], r);
}


static void parse_text_block(BoxList *boxlist, List *l, LinewiseReader r)
{
    const char *s = linewise_read(r);
    int n;
    int i;

    while (strncmp(s, ocrad_line_counter, ocrad_line_counter_length))
        s = linewise_read(r);

    n = atoi(s + ocrad_line_counter_length);
    list_init(l, LIST_BLOCK, n);
    
    for (i = 0; i < n; i++)
        parse_line(boxlist, &l->items[i], r);
}


/* Parse the "OCR Results File" produced by GNU Ocrad (with -x <filename> option). */
BoxList *parse_ocrad(List *l, const char *path)
{
    LinewiseReader r;
    BoxList *boxlist;
    
    const char *s = "";
    int n;
    int i;
    
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;
    
    r = linewise_reader_create(f);
    boxlist = box_list_create();
        
    while (strncmp(s, ocrad_block_counter, ocrad_block_counter_length))
        s = linewise_read(r);
 
    n = atoi(s + ocrad_block_counter_length);
    list_init(l, LIST_PAGE, n);

    for (i = 0; i < n; i++)
        parse_text_block(boxlist, &l->items[i], r);
    
    linewise_reader_destroy(r);
    list_resolve(l, boxlist->boxes);
    box_list_finalize(boxlist);
    return boxlist;
}

