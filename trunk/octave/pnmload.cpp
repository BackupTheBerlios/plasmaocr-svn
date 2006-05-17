#if 0
#//   This is an Octave binding for pnmload().
#//   Run sh on this file to compile it.
#//   The following commands will be executed:

    mkoctfile -v -I../core pnmload.cpp ../core/bitmaps.c ../src/pnm.c
    exit
#endif

    
#include "bridge_to_octave.h"
#include "bitmaps.h"
#include <stdio.h>

// KLUGE
extern "C" int load_pnm(FILE *f, unsigned char ***pixels, int *w, int *h);

DEFUN_DLD (pnmload, args, , "[data, PNM_type] = pnmload(filename)\n"
                            "Load a PNM file; PNM_type is 4,5 or 6.\n"
                            "Supports only maxval 255.\n")
{
    const char *path = args(0).string_value().c_str();
    unsigned char **pixels;
    int w, h;
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror(path);
        return octave_value_list();
    }
    int type = load_pnm(f, &pixels, &w, &h);
    fclose(f);

    octave_value loaded = bitmap_to_octave(pixels, w, h);
    free_bitmap(pixels);
    
    octave_value_list result;
    result(0) = loaded;
    result(1) = type;
    
    return result;
}

