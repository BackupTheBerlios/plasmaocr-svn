/* Plasma OCR - an OCR engine
 *
 * linewise.c - reading text files line by line
 *
 * Copyright (C) 2006  Ilya Mezhirov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#include <stdlib.h>
#include "linewise.h"


#define INITIAL_BUFFER_SIZE 256
#define NEWLINE1 '\n'
#define NEWLINE2 '\r'


struct LinewiseReaderStruct
{
    FILE *in;                   /* the file; should be NULL after EOF */
    unsigned int line_size;
    char *line;
    char *line_end;             /* last char in the buffer */
};


LinewiseReader linewise_reader_create(FILE *in)
{
    LinewiseReader r = (LinewiseReader) malloc(sizeof(struct LinewiseReaderStruct));
    r->in = in;
    r->line_size = INITIAL_BUFFER_SIZE;
    r->line = (char *) malloc(INITIAL_BUFFER_SIZE * sizeof(char));
    r->line_end = r->line + r->line_size - 1;
    return r;
}


void linewise_reader_destroy(LinewiseReader r)
{
    free(r->line);
    free(r);
}


const char *linewise_read_and_chomp(LinewiseReader r)
{
    const char *result;
    char *p;
    result = linewise_read(r);
    if (!result)
        return NULL;

    /* Search for the newline and clear it */
    p = r->line;
    while (*p  &&  *p != NEWLINE1  &&  *p != NEWLINE2)
        p++;
    
    if (*p) *p = '\0';
    return result;
}


static void more_mem(LinewiseReader r)
{
    r->line_size <<= 1;
    r->line = (char *) realloc(r->line, r->line_size);
    r->line_end = r->line + r->line_size - 1;
}


const char *linewise_read(LinewiseReader r)
{
    char *start = r->line;

    if (!r->in) return NULL;

    while(1)
    {
        char *end = r->line_end;
        char *p;
        *end = 'X'; /* to know if we've reached the end of the buffer */
        p = fgets(start, end - start + 1, r->in);
        
        if (!p) /* EOF */
        {
            r->in = NULL;
            
            if (start == r->line)
                return NULL;    /* we haven't read anything at all */
            else
                return r->line; /* we've read something before, so return it */
        }
        
        if (*r->line_end) 
            return r->line; /* the buffer was enough (our 'X' is untouched) */
        
        if (*(r->line_end-1) == NEWLINE1 || *(r->line_end-1) == NEWLINE2)
            return r->line; /* the buffer is full */
        
        /* Symbols are coming, we need to call fgets() again. */
        start = r->line; /* now end - start == r->line_size - 1 */
        more_mem(r);     /* note: line may be moved */
        start = r->line + (end - start); /* now start points to that '\0' */
    } /* while(1) */
}
