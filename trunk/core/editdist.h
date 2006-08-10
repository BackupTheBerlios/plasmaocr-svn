#ifndef PLASMA_OCR_EDITDIST_H
#define PLASMA_OCR_EDITDIST_H


int edit_distance(int radius, const char *s1, int n1, const char *s2, int n2);

#ifdef TESTING
extern TestSuite editdist_suite;
#endif

#endif
