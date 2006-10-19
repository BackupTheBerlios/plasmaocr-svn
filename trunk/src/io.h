#ifndef IO_H
#define IO_H


#include <stdio.h>

void write_int32(int i, FILE *f);
int read_int32(FILE *f);

FILE *checked_fopen(const char *path, const char *mode);
void checked_fclose(FILE *);


#ifdef TESTING

typedef struct FilePairStruct *FilePair;

FilePair file_pair_open(void);
FILE *file_pair_read(FilePair);
FILE *file_pair_write(FilePair);
void file_pair_close(FilePair);

extern TestSuite io_suite;

#endif

#endif
