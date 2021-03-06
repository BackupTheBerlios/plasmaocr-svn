#ifndef PLASMA_OCR_CORE_H
#define PLASMA_OCR_CORE_H


#include "library.h"


typedef struct CoreStruct *Core;

Core create_core(void);
void free_core(Core);
void add_to_core(Core, Library l);
void set_core_orange_policy(Core, int level);
Library get_core_orange_library(Core);

typedef enum
{
    CC_RED,     /* what was that? */
    CC_GREEN,   /* certainly */
    CC_YELLOW,  /* just a guess */
    CC_BLUE     /* more than one "certain" match (which is strange) */
} ColorCode;


typedef struct
{
    ColorCode color;
    char *text;
    
    /* the following fields are only filled if need_explanations is set */
    LibraryIterator *best_match;
} RecognizedLetter;


RecognizedLetter *recognize_pattern(Core c, Pattern p, int need_explanation);

RecognizedLetter *recognize_letter(Core c,
                                   unsigned char **pixels, int width, int height,
                                   int need_explanation);

void free_recognized_letter(RecognizedLetter *);


typedef struct
{
    int count;        /* number of letters */
    char *text;
    RecognizedLetter **letters;    
} RecognizedWord;

RecognizedWord *recognize_word(Core c,
                               unsigned char **pixels, int width, int height,
                               int need_explanations);

void free_recognized_word(RecognizedWord *);


#endif
