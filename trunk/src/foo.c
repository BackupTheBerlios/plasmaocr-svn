#include "library.h"
#include "wordcut.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gd.h>

static gdImagePtr gdize(unsigned char **pixels, int width, int height)
{
    gdImagePtr img = gdImageCreate(width, height);
    int white = gdImageColorAllocate(img, 255, 255, 255);
    int black = gdImageColorAllocate(img, 0, 0, 0);
    int x, y;
    
    // The most shameful way to copy an image - by setting pixels...
    for (y = 0; y < height; y++) for (x = 0; x < width; x++)
        gdImageSetPixel(img, x, y, pixels[y][x] ? black : white);

    return img;
}

void cut(const char *out_name, unsigned char **pixels, int width, int height)
{
    int scale = 1;
    FILE *out = fopen(out_name, "wb");
    gdImagePtr img = gdize(pixels, width, height);
    gdImagePtr scaled = gdImageCreate(width * scale, height * scale);
    gdImageCopy(scaled, img, 0, 0, 0, 0, width, height);
    //int white = gdImageColorAllocate(img, 255, 255, 255);
    //int blue  = gdImageColorAllocate(img, 70, 70, 255);
    int red  = gdImageColorAllocate(scaled, 255, 0, 0);
    int gray  = gdImageColorAllocate(scaled, 127, 127, 127);
    //int black = gdImageColorAllocate(img, 0, 0, 0);
    int i;
    WordCut *c = cut_word(pixels, width, height);
    //gdImageCopyResized(scaled, img, 0, 0, 0, 0, width * scale, height * scale, width, height);
    gdImageDestroy(img);

    for (i = 0; i < c->count; i++)
    {
/*        int l = c->window_start[i] * scale - 1;
        int r = c->window_end[i] * scale + 1;
        int x, y;
        for (y = 0; y < height * scale; y++)
            for (x = l; x < r; x++)
        {
            int color = gdImageGetPixel(scaled, x, y);
            
            if (scaled->green[color] > 200)
                gdImageSetPixel(scaled, x, y, gray);
        }*/
        gdImageFilledRectangle(scaled, 
                               c->position[i] * scale, 0,
                               c->position[i] * scale + scale, height * scale,
                               red);
    }
    
    
    gdImagePng(scaled, out);

    fclose(out);
    gdImageDestroy(scaled);    
    destroy_word_cut(c);
}

int main(int argc, char **argv)
{
    Library l;
    LibraryIterator iter;
    int i;
    int c = 0;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <library>\n", argv[0]);
        exit(1);
    }

    l = library_open(argv[1]);
    library_read_prototypes(l);
    
    for (i = 0; i < library_shelves_count(l); i++)
    {
        char *x;
        char buf[20];
        Shelf *s = library_get_shelf(l, i);
        c++;
        sprintf(buf, "cut_%4d.png", c);
        for (x = buf; *x; x++) if (*x == ' ') *x = '0';
        cut(buf, s->pixels, s->width, s->height);
    }

    return 0;
}
