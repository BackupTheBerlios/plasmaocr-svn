#include "common.h"
#include "linewise.h"
#include <string.h>
#include <assert.h>


// Even though these structures seem very similar,
// they represent very different things and can be different
// in the future. So I won't merge them (that was done in 0.1)

typedef struct
{
    int left, top, width, height;
} LayoutWord;

typedef struct
{
    int count, allocated;
    LayoutWord *words;
} LayoutLine;

typedef struct
{
    int count, allocated;
    LayoutLine *lines;
} LayoutBlock;

typedef struct
{
    int count;
    LayoutBlock *blocks;
}  LayoutPage;

/* _____________________________   union of rectangles   _____________________________ */


static void union_rectangles(int *px, int *py, int *pw, int *ph,
                             int x, int y, int w, int h)
{
    if (*px > x)
    {
        *pw += *px - x;
        *px = x;
    }

    if (*py > y)
    {
        *ph += *py - y;
        *py = y;
    }

    if (x + w > *px + *pw)
        *pw = x + w - *px;

    if (y + h > *py + *ph)
        *ph = y + h - *py;
}


/* __________________________   freeing layout structures   __________________________ */


static void destroy_layout_line(LayoutLine *l)
{
    FREE(l->words);
}


static void destroy_layout_block(LayoutBlock *b)
{
    int i;
    for (i = 0; i < b->count; i++)
        destroy_layout_line(&b->lines[i]);
    if (b->lines) FREE(b->lines);
}


static void free_layout_page(LayoutPage *p)
{
    int i;
    for (i = 0; i < p->count; i++)
        destroy_layout_block(&p->blocks[i]);
    if (p->blocks) FREE(p->blocks);
    FREE1(p);
}


/* _______________________   parsing ORF (Ocrad Results File)   ______________________ */

const char ocrad_char_counter[] = "chars ";
int ocrad_char_counter_length = sizeof(ocrad_char_counter) - 1;


static void parse_box(LayoutWord *words, int *count, LinewiseReader r, int *start_word)
{
    const char *s = linewise_read(r);
    const char *first_quote;
    int x, y, w, h;
    
    assert(s);
    sscanf(s, "%d %d %d %d", &x, &y, &w, &h);
    first_quote = strchr(s, '\'');

    if (first_quote && first_quote[1] == ' ' /* is space */)
    {
        *start_word = 1;
        return;
    }

    if (*start_word)
    {
        *start_word = 0;
        words[*count].left = x;
        words[*count].top = y;
        words[*count].width = w;
        words[*count].height = h;
        (*count)++;
    }
    else
    {
        assert(*count > 0);
        union_rectangles(&words[*count-1].left,
                         &words[*count-1].top,
                         &words[*count-1].width,
                         &words[*count-1].height,
                         x, y, w, h);
    }
}

static void parse_line(LayoutLine *l, LinewiseReader r)
{
    const char *s = linewise_read(r);

    /* read a line like this:
     *      line 5 chars 1 height 3
     * the only interesting figure here is the number of chars
     */
    char *nchars;
    char *mark;
    int n;
    int i;
    int start_word = 1;
    l->count = 0;

    while (!(mark = strstr(s, ocrad_char_counter)))
    {
        s = linewise_read(r);
        if (!s)
        {
            fprintf(stderr, "orf2pjf> parse_line: cannot find line header\n");
            exit(1);
        }
    }
    nchars = mark + ocrad_char_counter_length;
    n = atoi(nchars);
    if (!n)
    {
        l->words = NULL;
        return;
    }
    
    l->words = MALLOC(LayoutWord, n);

    for (i = 0; i < n; i++)
    {
        parse_box(l->words, &l->count, r, &start_word);
        assert(l->count || start_word);
        assert(l->count <= n);
    }
    
    if (!l->count)
    {
        FREE(l->words);
        l->words = NULL;
    }
    else
    {
        l->words = REALLOC(LayoutWord, l->words, l->count);
    }
}

const char ocrad_line_counter[] = "lines ";
int ocrad_line_counter_length = sizeof(ocrad_line_counter) - 1;

static void parse_text_block(LayoutBlock *b, LinewiseReader r)
{
    const char *s = linewise_read(r);
    int n;
    int i;

    while (strncmp(s, ocrad_line_counter, ocrad_line_counter_length))
        s = linewise_read(r);

    n = atoi(s + ocrad_line_counter_length);
    b->count = n;
    b->lines = MALLOC(LayoutLine, n);
    
    for (i = 0; i < n; i++)
        parse_line(&b->lines[i], r);
}

static const char ocrad_block_counter[] = "total text blocks ";
static const int  ocrad_block_counter_length = sizeof(ocrad_block_counter) - 1;

static LayoutPage *parse_ocrad_results(FILE *f)
{
    LinewiseReader r;
    const char *s = "";
    LayoutPage *result;
    int i, n;

    r = linewise_reader_create(f);

    while (s && strncmp(s, ocrad_block_counter, ocrad_block_counter_length))
        s = linewise_read(r);

    n = atoi(s + ocrad_block_counter_length);
    result = MALLOC1(LayoutPage);
    result->count = n;
    result->blocks = MALLOC(LayoutBlock, n);
    
    for (i = 0; i < n; i++)
        parse_text_block(&result->blocks[i], r);

    linewise_reader_destroy(r);
    return result;
}


/* _______________________________   printing PJF   _________________________________ */

static void print_pjf_word(LayoutWord *w, FILE *f)
{
    fprintf(f, "$word %d %d %d %d$", w->left, w->top, w->width, w->height);
}

static void print_pjf_line(LayoutLine *l, FILE *f)
{
    int i;
    for (i = 0; i < l->count; i++)
    {
        if (i)
            fputc(' ', f);

        print_pjf_word(&l->words[i], f);
    }
    fputc('\n', f);
}

static void print_pjf_block(LayoutBlock *b, FILE *f)
{
    int i;
    for (i = 0; i < b->count; i++)
        print_pjf_line(&b->lines[i], f);
}

static void print_pjf_page(LayoutPage *p, FILE *f)
{
    int i;
    for (i = 0; i < p->count; i++)
    {
        if (i)
            fputc('\n', f);

        print_pjf_block(&p->blocks[i], f);
    }
}

/* __________________________________   main   ______________________________________ */

int main(int argc, char **argv)
{
    FILE *in = stdin, *out = stdout;
    LayoutPage *page;
    
    if (argc > 1)
    {
        in = fopen(argv[1], "r");
        if (!in)
        {
            perror(argv[1]);
            exit(1);
        }
    }

    if (argc > 2)
    {
        out = fopen(argv[2], "w");
        if (!out)
        {
            perror(argv[2]);
            exit(1);
        }        
    }
    
    page = parse_ocrad_results(in);
    print_pjf_page(page, out);
    free_layout_page(page);
    
    if (in != stdin)
        fclose(in);

    if (out != stdout)
        fclose(out);
    
    return 0;
}
