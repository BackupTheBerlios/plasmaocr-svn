#include "common.h"
#include "io.h"


void write_int32(int i, FILE *f)
{
    fputc(i >> 24, f);
    fputc(i >> 16, f);
    fputc(i >> 8, f);
    fputc(i, f);    
}


int read_int32(FILE *f)
{
    int i = fgetc(f);
    i = (i << 8) | fgetc(f);
    i = (i << 8) | fgetc(f);
    i = (i << 8) | fgetc(f);
    return i;
}
