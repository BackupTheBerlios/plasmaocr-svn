#include "bitmaps.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>


unsigned char **allocate_bitmap(int w, int h)
{
    unsigned char *data, **result;
    int i;

    assert(w > 0 && h > 0);

    data = MALLOC(unsigned char, w * h);
    if (!data) return NULL;

    result = MALLOC(unsigned char *, h);
    if (!result)
    {
        FREE(data);
        return NULL;
    }

    for (i = 0; i < h; i++)
    {
        result[i] = data + w * i;
    }

    return result;
}


void free_bitmap(unsigned char **p)
{
    assert(p);
    FREE(p[0]);
    FREE(p);
}


unsigned char **allocate_bitmap_with_margins(int w, int h)
{
    unsigned char **result = allocate_bitmap(w + 2, h + 2);
    int y;

    if (!result)
        return NULL;

    /* set the `result' array so that it points to buffer rows (plus margin) */
    for (y = 0; y < h + 2; y++)
    {
        result[y]++;
    }
    result++; /* now the first byte in the buffer is result[-1][-1] */
    return result;
}


/* Allocate a w * h bitmap with margins of 1 pixels at each side.
 * Copy `pixels' there and clear the margins.
 */
unsigned char **provide_margins(unsigned char **pixels,
                                int w, int h,
                                int make_it_0_or_1)
{
    unsigned char **result = allocate_bitmap_with_margins(w, h);
    int y;

    if (!result)
        return NULL;

    /* clear the top and the bottom row */
    memset(result[-1] - 1, 0, w + 2);
    memset(result[h]  - 1, 0, w + 2);

    for (y = 0; y < h; y++)
    {
        unsigned char *src_row = pixels[y];
        unsigned char *dst_row = result[y];

        /* clear left and right margin */
        dst_row[-1] = 0;
        dst_row[w]  = 0;

        if (!make_it_0_or_1)
            memcpy(dst_row, src_row, w);
        else
        {
            int x;
            for (x = 0; x < w; x++)
                dst_row[x] = (src_row[x] ? 1 : 0);
        }
    }

    return result;
}


/* Simply undo the work of allocate_bitmap_with_margin(). */
void free_bitmap_with_margins(unsigned char **pixels)
{
    assert(pixels);
    FREE(&pixels[-1][-1]); /* because we have both left and top margins of 1 pixel */
    FREE(pixels - 1);
}


void assign_bitmap(unsigned char **dst, unsigned char **src, int w, int h)
{
    int i;
    for (i = 0; i < h; i++)
        memcpy(dst[i], src[i], w);
}


unsigned char **copy_bitmap(unsigned char **src, int w, int h)
{
    unsigned char **result = allocate_bitmap(w, h);
    assign_bitmap(result, src, w, h);
    return result;
}


void strip_endpoints_4(unsigned char **result, unsigned char **pixels, int w, int h)
{
    int x, y;

    assert(result != pixels);

    clear_bitmap(result, w, h);

    for (y = 0; y < h; y++) for (x = 0; x < w; x++) if (pixels[y][x])
    {
        int degree = pixels[y - 1][x] + pixels[y + 1][x]
                   + pixels[y][x - 1] + pixels[y][x + 1];

        if(degree != 1)
            result[y][x] = 1;
    }
}

void strip_endpoints_8(unsigned char **result, unsigned char **pixels, int w, int h)
{
    int x, y;

    assert(result != pixels);

    clear_bitmap(result, w, h);

    for (y = 0; y < h; y++) for (x = 0; x < w; x++) if (pixels[y][x])
    {
        int degree = pixels[y - 1][x - 1] + pixels[y - 1][x] + pixels[y - 1][x + 1]
                   + pixels[y    ][x - 1] +                    pixels[y    ][x + 1]
                   + pixels[y + 1][x - 1] + pixels[y + 1][x] + pixels[y + 1][x + 1];

        if(degree != 1)
            result[y][x] = 1;
    }
}


int bitmaps_equal(unsigned char **p1, unsigned char **p2, int w, int h)
{
    int x, y;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            if (p1[y][x] != p2[y][x])
                return 0;
        }
    }
    return 1;
}


void print_bitmap(unsigned char **pixels, int w, int h)
{
    int x, y;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            putchar(pixels[y][x] ? '@' : '.');
        }
        putchar('\n');
    }
}


void make_bitmap_0_or_1(unsigned char **pixels, int w, int h)
{
    int x, y;

    for (y = 0; y < h; y++)
    {
        unsigned char *row = pixels[y];

        for (x = 0; x < w; x++)
            row[x] = ( row[x] ? 1 : 0 );
    }
}


void invert_bitmap(unsigned char **pixels, int w, int h, int first_make_it_0_or_1)
{
    int x, y;

    for (y = 0; y < h; y++)
    {
        unsigned char *row = pixels[y];

        if (first_make_it_0_or_1)
        {
            for (x = 0; x < w; x++)
                row[x] = ( row[x] ? 0 : 1 );
        }
        else
        {
            for (x = 0; x < w; x++)
                row[x] = 1 - row[x];
        }
    }
}


unsigned char **allocate_bitmap_with_white_margins(int w, int h)
{
    unsigned char **result = allocate_bitmap_with_margins(w, h);
    int y;

    memset(result[-1] - 1, 0, w + 2);
    memset(result[ h] - 1, 0, w + 2);
    for (y = 0; y < h; y++)
    {
        result[y][-1] = 0;
        result[y][w] = 0;
    }

    return result;
}


void clear_bitmap(unsigned char **pixels, int w, int h)
{
    int y;

    for (y = 0; y < h; y++)
        memset(pixels[y], 0, w);
}


int find_mass(unsigned char **pixels, int w, int h)
{
    int sum = 0;
    int i, j;
    for (i = 0; i < h; i++)
    {
        unsigned char *row = pixels[i];
        for (j = 0; j < w; j++)
        {
            if (row[j])
                sum++;
        }
    }
    return sum;
}


void find_mass_center(unsigned char **pixels, int w, int h, int *m, int *cx, int *cy, int quant)
{
    int x, y;
    int sx = 0, sy = 0;
    int mass;

    if (m && *m)
        mass = *m;
    else
        mass = find_mass(pixels, w, h);

    for (y = 0; y < h; y++)
    {
        unsigned char *row = pixels[y];
        for (x = 0; x < w; x++)
        {
            if (row[x])
            {
                sx += x;
                sy += y;
            }
        }
    }

    if (cx) *cx = quant * sx / mass;
    if (cy) *cy = quant * sy / mass;
    if (m) *m = mass;
}


#ifdef TESTING

/* Try writing into the pointer array and the bitmap.
 * If something nasty happens, Valgrind should print an error.
 */
static void try_to_write_into_bitmap(unsigned char **bitmap, int w, int h)
{
    int i, j;

    assert(bitmap);
    for (i = 0; i < h; i++)
    {
        unsigned char *save = bitmap[i];
        assert(save);
        bitmap[i] = NULL;
        for (j = 0; j < w; j++)
        {
            unsigned char save_pixel = save[j];
            save[j] = 0;
            save[j] = save_pixel;
        }
        bitmap[i] = save;
    }
}


/* Check that the bitmap is in fact a contiguous memory block. */
static void test_bitmap_integrity(unsigned char **bitmap, int w, int h)
{
    int i;
    try_to_write_into_bitmap(bitmap, w, h);
    for (i = 0; i < h; i++)
    {
        assert(bitmap[i] == bitmap[0] + w * i);
    }
}

static void allocate_bitmap_basic_test()
{
    int w = 10;
    int h = 10;
    unsigned char **bitmap = allocate_bitmap(w, h);
    test_bitmap_integrity(bitmap, w, h);
    free_bitmap(bitmap);    
}


static TestFunction tests[] = {
    allocate_bitmap_basic_test,
    NULL
};

TestSuite bitmaps_suite = {"bitmaps", NULL, NULL, tests};


#endif /* TESTING */
