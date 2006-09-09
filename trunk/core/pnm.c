/* Plasma OCR - an OCR engine
 *
 * pnm.c - loading PNM files
 *
 * Copyright (C) 2006  Ilya Mezhirov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "pnm.h"
#include "bitmaps.h"
#include "memory.h"
#include <assert.h>


static void skip_comment_line(FILE *file)
{
    while (1) switch(fgetc(file))
    {
        case EOF: case '\r': case '\n':
            return;
    }
}


static void skip_whitespace_and_comments(FILE *file)
{
    int c = fgetc(file);
    while(1) switch(c)
    {
        case '#':
            skip_comment_line(file);
            /* fall through */
        case ' ': case '\t': case '\r': case '\n':
            c = fgetc(file);
        break;
        default:
            ungetc(c, file);
            return;
    }
}


static void unpack(unsigned char *packed, unsigned char *result, int n)
{
    int coef = 0x80;
    int a = *packed;
    
    while (n--)
    {
        *result++  =  a & coef ? 1 : 0;

        coef >>= 1;
        if (!coef)
        {
            coef = 0x80;
            if (!n) break; /* FIXME: this is not necessary, only to valgrind */
            a = *++packed;
        }
    }
}


#define PBM_ROW_SIZE(W) (((W) + 7) >> 3)

static void load_pbm_raster(FILE *f, unsigned char **pixels, int w, int h)
{
    int i;
    int n = PBM_ROW_SIZE(w);
    unsigned char *row = MALLOC(unsigned char, n);
    for (i = 0; i < h; i++)
    {
        if (fread(row, 1, n, f) != (unsigned) n)
        {
            fprintf(stderr, "problem in PBM file raster\n");
            exit(1);
        }
        unpack(row, pixels[i], w);
    }
    FREE(row);
}


int load_pnm_from_FILE(FILE *f, unsigned char ***pixels, int *width, int *height)
{
    int maxval;
    char type;

    if (getc(f) != 'P')
    {
        fprintf(stderr, "PNM file not starts with 'P'\n");
        exit(1);
    }
    
    type = getc(f);
    skip_whitespace_and_comments(f);
    
    switch(type)
    {
        case '4':
            fscanf(f, "%d %d", width, height);
            maxval = 255;
        break;
        case '5': case '6':
            fscanf(f, "%d %d %d", width, height, &maxval);
        break;
        default:
            fprintf(stderr, "only raw PNM files supported\n");
            exit(1);
            
    }

    if (maxval != 255)
    {
        fprintf(stderr, "only 256-levels PNM supported\n");
        exit(1);
    }

    switch(fgetc(f))
    {
        case ' ': case '\t': case '\r': case '\n':
            break;
        default:
            fprintf(stderr, "corrupted PNM, current offset is %ld\n", ftell(f));
            exit(1);
    }

    switch(type)
    {
        case '4':
            *pixels = allocate_bitmap(*width, *height);
            load_pbm_raster(f, *pixels, *width, *height);
            return PBM;
        case '5':
            *pixels = allocate_bitmap(*width, *height);
            fread(**pixels, *width, *height, f);
            return PGM;
        case '6':            
            *pixels = allocate_bitmap(*width * 3, *height);
            fread(**pixels, *width * 3, *height, f);
            return PPM;
        default:
            assert(0);
            return 0;
    }
}


int load_pnm(const char *path, unsigned char ***pixels, int *w, int *h)
{
    FILE *f = fopen(path, "rb");
    int result;
    if (!f)
    {
        perror(path);
        exit(1);
    }

    result = load_pnm_from_FILE(f, pixels, w, h);

    fclose(f);

    return result;
}
