/* Plasma OCR - an OCR engine
 *
 * code2tex.c - load and lookup in InftyCDB_code -> TeX transition table
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


#include "memory.h"
#include "linewise.h"
#include "code2tex.h"
#include <string.h>
#include <assert.h>


typedef struct
{
    int code;
    char TeX[32];
} Entry;


struct TransitionTableStruct
{
    Entry *entries;
    int count;
    int allocated;
};


static Entry *append(TransitionTable table)
    LIST_APPEND(Entry, table->entries, table->count, table->allocated)


static TransitionTable create_table()
{
    TransitionTable result = MALLOC1(struct TransitionTableStruct);
    result->count = 0;
    result->allocated = 10;
    result->entries = MALLOC(Entry, result->allocated);
    return result;
}
    

// FIXME: copied from ../test/inftycdb.c
static int char_hex2dec(char c)
{
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
    {
        assert(c >= '0' && c <= '9');
        return c - '0';
    }   
}


static unsigned short get_code(const char *code)
{
    return (char_hex2dec(code[0]) << 12)
         | (char_hex2dec(code[1]) << 8)
         | (char_hex2dec(code[2]) << 4)
         | char_hex2dec(code[3]);
}

    
static TransitionTable load_from_reader(LinewiseReader r)
{
    TransitionTable result = create_table();
    const char *line;
    
    while ((line = linewise_read_and_chomp(r)))
    {
        Entry *e = append(result);
        char *p = strchr(line, '\t');
        if (!p)
        {
            fprintf(stderr, "the code->TeX table has a line without tab!\n");
            exit(1);
        }
        *p = '\0';
        p++;
        
        if (strlen(p) + 1 > sizeof(e->TeX))
        {
            fprintf(stderr, "the code->TeX table has too long (> %u) second field\n", sizeof(e->TeX));
            exit(1);
        }
        
        e->code = get_code(line);
        strcpy(e->TeX, p);
    }

    return result;
}

static int compare_entries(const void *p1, const void *p2)
{
    const Entry *e1 = (Entry *) p1;
    const Entry *e2 = (Entry *) p2;
    return e1->code - e2->code;
}


TransitionTable transition_table_load(const char *path)
{
    FILE *f = fopen(path, "r");
    LinewiseReader r;
    TransitionTable result;

    if (!f)
    {
        perror(path);
        exit(1);
    }
    
    r = linewise_reader_create(f);
    result = load_from_reader(r);
    qsort(result->entries, result->count, sizeof(Entry), compare_entries);
    linewise_reader_destroy(r);
    fclose(f);
    return result;
}


static const char *find_in_table(Entry *entries, int n, int code)
{
    int mid = n / 2;
    int midcode;
    
    if (!n)
        return NULL;
    
    midcode = entries[mid].code;

    if (midcode == code)
        return entries[mid].TeX;
    
    if (n == 1)
        return NULL;
    
    if (midcode < code)
        return find_in_table(entries + mid, n - mid, code);
    else
        return find_in_table(entries, mid, code);
}


const char *transition_table_lookup(TransitionTable table, int code)
{
    const char *p = find_in_table(table->entries, table->count, code);
    if (p) return p;
    p = find_in_table(table->entries, table->count, code | 0x2000);
    if (p) return p;
    p = find_in_table(table->entries, table->count, code | 0x4000);
    if (p) return p;
    p = find_in_table(table->entries, table->count, code | 0x6000);
    return p;
}


void transition_table_destroy(TransitionTable t)
{
    FREE(t->entries);
    FREE1(t);
}
