#ifndef PLASMA_OCR_RLE_H
#define PLASMA_OCR_RLE_H


void rle_encode_raw(FILE *, unsigned char *pixels, int n);
void rle_encode_FILE(FILE *, unsigned char **pixels, int w, int h);
void rle_encode(const char *path, unsigned char **pixels, int w, int h);

void rle_decode_raw(FILE *, unsigned char *pixels, int n);
void rle_decode_FILE(FILE *, unsigned char ***pixels, int *w, int *h);
void rle_decode(const char *path, unsigned char ***pixels, int *w, int *h);


#endif
