#include "common.h"
#include "library.h"
#include "io.h"
#include "bitmaps.h"
#include "rle.h"
#include <assert.h>


void library_iterator_init(LibraryIterator *li, int n, Library *libs)
{
    li->libraries_count = n;
    li->libraries = libs;
    li->current_library_index = 0;
    li->current_shelf_index = 0;

    if (n && library_shelves_count(*libs))
        li->current_shelf = library_get_shelf(*libs, 0);
    else
        li->current_shelf = NULL;

    li->current_record_index = 0;
}

LibraryRecord *library_iterator_next(LibraryIterator *li)
{
    Library l;
    LibraryRecord *result;
    if (li->current_library_index >= li->libraries_count)
        return NULL;
    
    assert(li->current_library_index >= 0);
    result = &li->current_shelf->records[li->current_record_index];
    
    l = li->libraries[li->current_library_index];

    li->current_record_index++;
    if (li->current_record_index == li->current_shelf->count)
    {
        /* change shelf */
        li->current_record_index = 0;
        li->current_shelf_index++;
        if (li->current_shelf_index == library_shelves_count(l))
        {
            /* change library */
            li->current_library_index++;
            l = li->libraries[li->current_library_index];
            li->current_shelf_index = 0;
            if (li->current_library_index == li->libraries_count)
            {
                /* finish */
                li->current_library_index = li->libraries_count;
                return result;
            }
        }
        li->current_shelf = library_get_shelf
                                (l, li->current_shelf_index);
    }
    return result;
}


struct LibraryStruct
{
    int count;
    int allocated;
    Shelf *shelves;
    FILE *file;
};


static void library_init(Library l)
    LIST_CREATE(Shelf, l->shelves, l->count, l->allocated, 16)

    
static Shelf *library_append_shelf(Library l)
    LIST_APPEND(Shelf, l->shelves, l->count, l->allocated)



/* _______________________   loading/saving records   _________________________ */

/* The record is stored in this way:
 *
 *  - pattern (see pattern.h)
 *  - radius (4 bytes)
 *  - text (1 to MAX_TEXT_SIZE bytes, zero-trailed)
 * 
 * Note that the box (left, top, width, height) is not stored,
 * because it's irrelevant for a batch run.
 * The box table is stored together with the prototype image.
 */


static void load_record_cache(LibraryRecord *lr, FILE *f)
{
    lr->pattern = load_pattern(f);
    promote_pattern(lr->pattern);
}

static void load_record_data(LibraryRecord *lr, FILE *f)
{
    int i;
    lr->disabled = 0;
    lr->radius = read_int32(f);
    for (i = 0; i < MAX_TEXT_SIZE; i++)
        if (!(lr->text[i] = fgetc(f))) break;
}

static void save_record_data(LibraryRecord *lr, FILE *f)
{
    int i;
    write_int32(lr->radius, f);
    for (i = 0; i < MAX_TEXT_SIZE; i++)
    {
        fputc(lr->text[i], f);
        if (!lr->text[i]) break;
    }
}

static void save_record_cache(LibraryRecord *lr, FILE *f)
{
    save_pattern(lr->pattern, f);
}

/* _______________________   loading/saving shelves   _________________________ */


static int load_shelf(Library l, FILE *f)
{
    int proto_size = read_int32(f);
    /* int data_size = */ read_int32(f);
    /* int cache_size = */ read_int32(f);
    int count = read_int32(f);
    int i;
    Shelf *s;

    
    if (feof(f)) return 0;
    /*if (proto_size <= 0)
    {
        fprintf(stderr,
                "The library seems corrupted or has original images missing.\n"
                "This is so strange that I quit.\n");
        exit(1);
    }*/
    s = library_append_shelf(l);
    s->offset_in_file = ftell(f);
    fseek(f, proto_size, SEEK_CUR);
    
    s->allocated = s->count = count;
    s->pixels = NULL;
    s->records = MALLOC(LibraryRecord, s->count);
    
    for (i = 0; i < s->count; i++)
        load_record_data(&s->records[i], f);
    for (i = 0; i < s->count; i++)
        load_record_cache(&s->records[i], f);

    return 1;
}


static void load_rectangles(Shelf *s, FILE *f)
{
    int i;
    for (i = 0; i < s->count; i++)
    {
        s->records[i].left   = read_int32(f);
        s->records[i].top    = read_int32(f);
        s->records[i].width  = read_int32(f);
        s->records[i].height = read_int32(f);
    }    
}


static int load_shelf_recreating(Library l, FILE *f)
{
    int proto_size = read_int32(f);
    /* int data_size = */ read_int32(f);
    int cache_size = read_int32(f);
    int count = read_int32(f);
    int i;
    Shelf *s;
    PatternCache pc;

    
    if (feof(f)) return 0;
    /*if (proto_size <= 0)
    {
        fprintf(stderr,
                "The library seems corrupted or has original images missing.\n"
                "This is so strange that I quit.\n");
        exit(1);
    }*/
    s = library_append_shelf(l);
    s->offset_in_file = ftell(f);
    //fseek(f, proto_size, SEEK_CUR);
    
    s->allocated = s->count = count;
    s->pixels = NULL;
    s->records = MALLOC(LibraryRecord, s->count);

    rle_decode_FILE(f, &s->pixels, &s->width, &s->height);
    pc = create_pattern_cache(s->pixels, s->width, s->height);

    load_rectangles(s, f);
    
    for (i = 0; i < count; i++)
    {
        s->records[i].pattern = create_pattern_from_cache(s->pixels, s->width, s->height,
                s->records[i].left, s->records[i].top,
                s->records[i].width, s->records[i].height, pc);
    }
    destroy_pattern_cache(pc);

    if (ftell(f) != s->offset_in_file + proto_size)
    {
        fprintf(stderr, "Library loading failed (even in pattern recreation mode)\n");
        fprintf(stderr, "ftell is %ld, offset_in_file is %ld, proto_size is %d\n", ftell(f), s->offset_in_file, proto_size);
        exit(1);
    }

    for (i = 0; i < s->count; i++)
        load_record_data(&s->records[i], f);

    fseek(f, cache_size, SEEK_CUR);
    
    return 1;
}



static void shelf_load_prototype(Shelf *s, FILE *f)
{
    rle_decode_FILE(f, &s->pixels, &s->width, &s->height);
    s->ownership = 1;
    load_rectangles(s, f);
}

static void shelf_save_prototype(Shelf *s, FILE *f)
{
    int i;
    assert(s->pixels);
    rle_encode_FILE(f, s->pixels, s->width, s->height);
    for (i = 0; i < s->count; i++)
    {
        write_int32(s->records[i].left,   f);
        write_int32(s->records[i].top,    f);
        write_int32(s->records[i].width,  f);
        write_int32(s->records[i].height, f);
    }
}



static void save_shelf(Shelf *s, FILE *f)
{
    long pos0, pos1, pos2, pos3, pos4;
    int i;
    pos0 = ftell(f);
    write_int32(0, f); /* to be overwritten later */
    write_int32(0, f); /* to be overwritten later */
    write_int32(0, f); /* to be overwritten later */
    write_int32(s->count, f);
    pos1 = ftell(f);
    shelf_save_prototype(s, f);
    pos2 = ftell(f);
    for (i = 0; i < s->count; i++)
        save_record_data(&s->records[i], f);
    pos3 = ftell(f);
    for (i = 0; i < s->count; i++)
        save_record_cache(&s->records[i], f);
    pos4 = ftell(f);
    fseek(f, pos0, SEEK_SET);
    write_int32(pos2 - pos1, f);
    write_int32(pos3 - pos2, f);
    write_int32(pos4 - pos3, f);
    fseek(f, pos4, SEEK_SET);
}


static void shelf_init(Shelf *s)
    LIST_CREATE(LibraryRecord, s->records, s->count, s->allocated, 8)
    
LibraryRecord *shelf_append(Shelf *s)
    LIST_APPEND(LibraryRecord, s->records, s->count, s->allocated)


Shelf *shelf_create(Library l)
{
    Shelf *s = library_append_shelf(l);
    shelf_init(s);
    s->ownership = 0;
    return s;
}


    
Library library_create()
{
    Library l = MALLOC1(struct LibraryStruct);
    library_init(l);
    l->file = NULL;
    return l;
}


Library library_open(const char *path)
{
    Library l = library_create();
    FILE *f = checked_fopen(path, "rb");
    
    l->file = f;
    while (1)
        if (!load_shelf(l, f)) break;
    
    return l;
}
Library library_open_recreating(const char *path)
{
    Library l = library_create();
    FILE *f = checked_fopen(path, "rb");
    
    l->file = NULL;
    while (1)
        if (!load_shelf_recreating(l, f)) break;
    
    checked_fclose(f);
    
    return l;
}


void library_read_prototypes(Library l)
{
    FILE *f = l->file;
    int i;
    for (i = 0; i < l->count; i++)
    {
        fseek(f, l->shelves[i].offset_in_file, SEEK_SET);
        shelf_load_prototype(&l->shelves[i], f);
    }
    checked_fclose(l->file);
    l->file = NULL;
}


static void shelf_destroy(Shelf *s)
{
    int i;
    if (s->ownership)
        free_bitmap(s->pixels);

    for (i = 0; i < s->count; i++)
        free_pattern(s->records[i].pattern);

    FREE(s->records);
}


void library_free(Library l)
{
    int i;
    if (l->file)
        fclose(l->file);
    for (i = 0; i < l->count; i++)
        shelf_destroy(&l->shelves[i]);
    FREE(l->shelves);
    FREE1(l);
}


void library_discard_prototypes(Library l)
{
    fclose(l->file);
    l->file = NULL;
}


void library_save(Library l, const char *path, int append)
{
    /* +b is a special case handled by checked_fopen():
     * it's like r+b, but if file doesn't exist, it's created.
     */
    FILE *f = checked_fopen(path, append ? "+b" : "wb");
    int i;
    assert(!l->file);

    if (append)
        fseek(f, 0, SEEK_END);
        
    for (i = 0; i < l->count; i++)
        save_shelf(&l->shelves[i], f);

    checked_fclose(f);
}


int library_shelves_count(Library l)
{
    assert(l);
    return l->count;
}


Shelf *library_get_shelf(Library l, int i)
{
    return &l->shelves[i];
}
