#ifndef PLASMA_OCR_PATTERN_H
#define PLASMA_OCR_PATTERN_H


#include <stdio.h>


/* A pattern is a function of image that can be matched with other patterns.
 */
typedef struct PatternStruct *Pattern;
typedef struct PatternCacheStruct *PatternCache;


Pattern create_pattern(unsigned char **pixels, int width, int height);
void save_pattern(Pattern p, FILE *f);
Pattern load_pattern(FILE *f);
void free_pattern(Pattern);


/* PatternCaches present alternative way of creating a pattern.
 */
PatternCache create_pattern_cache(unsigned char **pixels, int width, int height);
Pattern create_pattern_from_cache(unsigned char **pixels, int width, int height,
                                  int left, int top, int w, int h, PatternCache p);
void destroy_pattern_cache(PatternCache);



typedef struct MatchStruct *Match;

void promote_pattern(Pattern);
Match match_patterns(Pattern p1, Pattern p2);
int compare_patterns(int radius, Match m, Pattern p1, Pattern p2, int *penalty);
long patterns_shiftcut_dist(Pattern p1, Pattern p2);
void destroy_match(Match);

int pattern_size_test(Pattern p1, Pattern p2);

#ifdef TESTING

void assert_patterns_equal(Pattern p1, Pattern p2);

extern TestSuite pattern_suite;

#endif

#endif
