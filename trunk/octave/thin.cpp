#if 0
#//   This is an Octave binding for thin().
#//   Run sh on this file to compile it.
#//   The following commands will be executed:

    mkoctfile -v -I../core thin.cpp ../core/bitmaps.c ../core/thinning.c
    exit
#endif


#include "bridge_to_octave.h"
#include "bitmaps.h"
#include "thinning.h"
#include <stdio.h>



DEFUN_DLD (thin, args, ,
"thin(A, N=1)\n"
"Morphological thinning/thickening/skeletonization.\n\n"
"The image should be an 2D uint8 array; the result is also uint8.\n\n"
"N > 0: thin a bitmap N times.\n"
"N = 0: skeletonize (thin until it stops changing).\n"
"N < 0: thicken a bitmap -N times.\n\n"
"Thinning does not change the size of the image.\n"
"Thickening enlarges the image by 1 pixel at each side,\n"
"thus increasing both dimensions by 2.\n"
)
{
    unsigned char **pixels;
    int w, h;
    bitmap_from_octave(args(0), &pixels, &w, &h);
    if (!w)
        return octave_value_list();

    int N = 1;
    if (args.length() > 1)
        N = args(1).int_value();

    unsigned char **output;
    octave_value result;
    
    if (N > 0)
    {
        output = thin(pixels, w, h, N);
        result = bitmap_to_octave(output, w, h);
    }
    else if (N == 0)
    {
        output = skeletonize(pixels, w, h, /* use_8_connectivity: */ 0);
        result = bitmap_to_octave(output, w, h);        
    }
    else // N < 0
    {
        output = thicken(pixels, w, h, -N);
        result = bitmap_to_octave(output, w - N * 2, h - N * 2);
    }
    
    free_bitmap_with_margins(output);
    free_bitmap(pixels);
    return result;
}
