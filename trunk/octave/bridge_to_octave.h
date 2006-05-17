#include <octave/oct.h>

#include "bitmaps.h"

void bitmap_from_octave(octave_value val, unsigned char ***pixels, int *w, int *h)
{
    uint8NDArray gm = val.uint8_array_value();
    
    if (gm.dims().length() != 2)
    {
        fprintf(stderr, "bitmap_from_octave> argument is not 2D (forgotten uint8()?)\n");
        *pixels = NULL;
        *w = 0;
        *h = 0;
        return;
    }

    *h = gm.dim1();
    *w = gm.dim2();
    *pixels = allocate_bitmap(*w, *h);

    for (int y = 0; y < *h; y++)
        for (int x = 0; x < *w; x++)
            (*pixels)[y][x] = octave_uint8(gm(y,x)).value();
}


octave_value bitmap_to_octave(unsigned char **pixels, int w, int h)
{
    uint8NDArray result(dim_vector(h, w));
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            result(y,x) = pixels[y][x];
    
    return octave_value(result);
}
