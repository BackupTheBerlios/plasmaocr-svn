#ifndef PLASMA_OCR_CORE_H
#define PLASMA_OCR_CORE_H


typedef struct PatternStruct *Pattern;
typedef struct MatchStruct *Match;


Pattern core_prepare(unsigned char **pixels, int width, int height);
void core_promote(Pattern);
Match core_match(Pattern p1, Pattern p2);
int core_compare(int radius, Match m, Pattern p1, Pattern p2);
void core_destroy_match(Match);
void core_destroy_pattern(Pattern);


#endif
