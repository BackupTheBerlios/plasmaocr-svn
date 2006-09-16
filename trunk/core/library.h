#ifndef PLASMA_OCR_LIBRARY_H
#define PLASMA_OCR_LIBRARY_H

#define DEFAULT_RADIUS 50
#define MAX_TEXT_SIZE 20

#include "pattern.h"

typedef struct LibraryStruct *Library;

typedef struct
{
    Pattern pattern;
    char text[MAX_TEXT_SIZE]; /* "" - unknown */
    int radius;
    int left, top, width, height;
    int disabled;
} LibraryRecord;


typedef struct
{
    LibraryRecord *records;
    int count;
    int allocated;
    unsigned char **pixels;
    int width, height;
    int ownership; /* if nonzero, `pixels' will be freed in the end */
    long offset_in_file;
} Shelf;


Shelf *shelf_create(Library);
LibraryRecord *shelf_append(Shelf *);

Library library_create(void);
Library library_open(const char *path);
void library_read_prototypes(Library);
void library_discard_prototypes(Library);
void library_free(Library);
void library_save(Library, const char *path);
int library_shelves_count(Library);
Shelf *library_get_shelf(Library, int i);


typedef struct
{
    int libraries_count;
    Library *libraries;
    int current_library_index;
    int current_shelf_index;
    Shelf *current_shelf;
    int current_record_index;
} LibraryIterator;

void library_iterator_init(LibraryIterator *, int nlibraries, Library *);
LibraryRecord *library_iterator_next(LibraryIterator *);


#endif
