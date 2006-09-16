#include "common.h"
#include "bitmaps.h"
#include "pnm.h"
#include "io.h"
#include "rle.h"
#include <string.h>
#include <assert.h>


#define MAX_WHITE_RUN 15
#define MAX_BLACK_RUN 15
#define MAGIC (('r' << 24) | ('l' << 16) | ('e' << 8) | '1')


void rle_encode_raw(FILE *f, unsigned char *pixels, int n)
{
    int i = 0;

    while (i < n)
    {
        int wrun = 0, brun = 0;
        
        /* gather white run */
        while (!pixels[i] && i < n && wrun < MAX_WHITE_RUN)
        {
            i++;
            wrun++;
        }
        while (pixels[i] && i < n && brun < MAX_BLACK_RUN)
        {
            i++;
            brun++;
        }

        fputc((wrun << 4) | brun, f);
    }
}


void rle_decode_raw(FILE *f, unsigned char *pixels, int n)
{
    int i = 0;
    assert(n > 0);
    while (i < n)
    {
        unsigned char c = fgetc(f);
        unsigned char w = c >> 4;
        unsigned char b = c & 0xF;
        if (i + w + b > n)
        {
            fprintf(stderr, "corrupted RLE encoding\n");
            exit(1);
        }

        memset(pixels + i, 0, w);
        i += w;
        memset(pixels + i, 1, b);
        i += b;
    }
}


void rle_encode_FILE(FILE *f, unsigned char **pixels, int w, int h)
{
    /* Well, that is a "suboptimal" way to encode,
     * but we don't care here for speed.
     */
    unsigned char *buf = MALLOC(unsigned char, w * h);
    int i;

    for (i = 0; i < h; i++)
        memcpy(buf + i * w, pixels[i], w);
    
    write_int32(MAGIC, f);
    write_int32(w, f);
    write_int32(h, f);
    rle_encode_raw(f, buf, w * h);

    FREE(buf);
}


void rle_encode(const char *path, unsigned char **pixels, int w, int h)
{
    if (!strcmp(path, "-"))
        rle_encode_FILE(stdout, pixels, w, h);
    else
    {
        FILE *f = fopen(path, "wb");
        if (!f)
        {
            perror(path);
            exit(1);
        }
        rle_encode_FILE(f, pixels, w, h);
        fclose(f);
    }
}


void rle_decode_FILE(FILE *f, unsigned char ***pixels, int *w, int *h)
{
    int magic = read_int32(f);
    *w = read_int32(f);
    *h = read_int32(f);

    if (magic != MAGIC || *w <= 0 || *h <= 0)
    {
        fprintf(stderr, "RLE encoded data expected, but not found\n");
        exit(1);
    }

    *pixels = allocate_bitmap(*w, *h);
    rle_decode_raw(f, (*pixels)[0], *w * *h);
}


void rle_decode(const char *path, unsigned char ***pixels, int *w, int *h)
{
    if (!strcmp(path, "-"))
        rle_decode_FILE(stdin, pixels, w, h);
    else
    {
        FILE *f = fopen(path, "rb");
        if (!f)
        {
            perror(path);
            exit(1);
        }
        rle_decode_FILE(f, pixels, w, h);
        fclose(f);
    }
}
