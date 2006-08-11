#ifndef PLASMA_OCR_CORE_H
#define PLASMA_OCR_CORE_H


#include <stdio.h>


typedef struct PatternStruct *Pattern;
typedef struct MatchStruct *Match;
typedef struct LibraryStruct *Library;

Pattern core_prepare(unsigned char **pixels, int width, int height);
void core_promote(Pattern);
Match core_match(Pattern p1, Pattern p2);
int core_compare(int radius, Match m, Pattern p1, Pattern p2, int *penalty);
void core_destroy_match(Match);
void core_destroy_pattern(Pattern);
void core_save_pattern(Pattern p, FILE *f);
Pattern core_load_pattern(FILE *f);

Library core_load_raw_library(const char *path);
Library core_load_library(const char *path);
void core_save_library(Library, const char *path);
void core_destroy_library(Library);

typedef enum
{
    CC_BLACK,   // only shallow match
    CC_RED,     // no shallow match
    CC_GREEN,   // only deep match
    CC_YELLOW,  // best shallow, no deep match
    CC_MAGENTA, // uncertainly recognized by two methods
    CC_BLUE     // more than one deep matches
} ColorCode;

typedef struct
{
    ColorCode color;
    const char *text;
} Opinion;

Opinion *core_recognize_letter(Library, int no_blacks, unsigned char **pixels, int width, int height);
void core_destroy_opinion(Opinion *);

#endif
