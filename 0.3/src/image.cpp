#include "image.h"

PLASMA_BEGIN;

#ifndef NDEBUG

void Image::assert_ok_parameters()
{
    assert(w > 0);
    assert(h > 0);
    assert(x_margin >= 0);
    assert(y_margin >= 0);

    // we can't both delete and free
    assert(!((flags & FREE_PIXELS) && (flags & DELETE_PIXELS)));
    assert(!((flags & FREE_POINTERS) && (flags & DELETE_POINTERS)));

    // freeing/deleting pixels is impossible without ONE_PIECE.
    assert(!(!(flags & ONE_PIECE) && 
                ((flags & FREE_PIXELS) || (flags & DELETE_PIXELS))));

    // ZERO_OR_ONE has sense only for bitmaps.
    assert(!(ZERO_OR_ONE && type != BITMAP)); 
}

#endif


Image(int width, int h, Type t, int f = 0, int x_m = 0, int y_m = 0):
    w(width), h(height), type(t),
    flags(f | ONE_PIECE | DELETE_PIXELS | DELETE_POINTERS),
    x_margin(x_m), y_margin(y_m)
{
    assert_ok_parameters();

    int full_s = row_size_with_margins();
    int full_h = height_with_margins();
    
    byte *buf = new byte[full_s * full_h];
    pixels = new (byte *)[full_h];

    for (int i = 0; i < full_h; i++)
        pixels[i] = buf + full_s * i + x_margin;

    pixels += y_margin;
}

Image(byte **p, int width, int height, Type t, int f = 0, int x_m = 0, int y_m = 0):
    pixels(p), w(width), h(height), type(t), flags(f), x_margin(x_m), y_margin(y_m)
{
    assert_ok_parameters();
}

~Image()
{
    assert_ok_parameters();

    if (flags & DELETE_PIXELS)
        delete [] pixels[-y_margin];
    if (flags & FREE_PIXELS)
        free(pixels[-y_margin]);
    
    if (flags & DELETE_POINTERS)
        delete [] pixels - y_margin;
    if (flags & FREE_POINTERS)
        free(pixels - y_margin);
}

Image *copy()
{
}

Image &operator =(const Image &)
{
    
}

void Image::invert()
{
    XXXXXXXXXX
}

void clear()
{
    int full_s = row_size_with_margins();

    for (int i = -y_margin; i < height + y_margin; i++)
        memset(pixels[i], 0, full_s);
}
    
void Image::force_0_or_1()
{
    assert(type == BITMAP);
    
    for (int y = 0; y < h; y++)
    {
        byte *row = pixels[y];
        for (int x = 0; x < w; x++)
            if (row[x]) row[x] = 1;
    }

    flags |= ZERO_OR_ONE;
}

bool operator ==(const Image &img)
{
    XXXXXXXXX
}

void Image::contains(const Rect &r)
{
    return Rect(0, 0, w, h).contains(r);
}


PLASMA_END;
