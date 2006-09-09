#ifndef PLASMA_OCR_WORDCUT_H
#define PLASMA_OCR_WORDCUT_H


typedef struct
{
    int count;
    int *position; // 1 .. w - 1
    unsigned char *level; // 1..max_level; the more the level, the stronger the cut
    unsigned char max_level;
    int *window_start;
    int *window_end;
} WordCut;

WordCut *cut_word(unsigned char **pixels, int w, int h);

void word_cut_destroy(WordCut *);


#endif
