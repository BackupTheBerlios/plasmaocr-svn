#if 0
#//   This is an Octave binding for thicken().
#//   Run sh on this file to compile it.
#//   The following commands will be executed:

    mkoctfile -v -I../core thicken.cpp ../core/bitmaps.c ../core/thinning.c
    exit
#endif


#include "bridge_to_octave.h"
#include "bitmaps.h"
#include "thinning.h"
#include <stdio.h>



DEFUN_DLD (thicken, args, , "thicken(A, N=1)\nThicken a bitmap N times.")
{
    unsigned char **pixels;
    int w, h;
    bitmap_from_octave(args(0), &pixels, &w, &h);
    if (!w)
        return octave_value_list();

    int N = 1;
    if (args.length() > 1)
        N = args(1).int_value();

    unsigned char **output = thicken(pixels, w, h, N);
    octave_value result = bitmap_to_octave(output, w + N * 2, h + N * 2);
    free_bitmap_with_margins(output);
    free_bitmap(pixels);
    return result;
}

