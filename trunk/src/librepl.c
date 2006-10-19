#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int counter = 0;
    Library l;
    LibraryIterator iter;
    LibraryRecord *rec;
    
    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <library> <find what> <replace to what>\n", argv[0]);
        exit(1);
    }

    l = library_open(argv[1]);
    library_read_prototypes(l);
    library_iterator_init(&iter, 1, &l);
    while ((rec = library_iterator_next(&iter)))
    {
        if (!strcmp(rec->text, argv[2]))
        {
            strncpy(rec->text, argv[3], MAX_TEXT_SIZE);
            counter++;
        }
    }

    printf("%d replaces\n", counter);
    library_save(l, argv[1], 0);
    return 0;
}
