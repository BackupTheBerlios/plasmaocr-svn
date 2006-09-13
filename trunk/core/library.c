#include "common.h"
#include "library.h"
#include "io.h"
#include "rle.h"


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



// _______________________   loading/saving records   _________________________

/* The record is stored in this way:
 *
 *  - pattern (see pattern.h)
 *  - radius (4 bytes)
 *  - text (1 to MAX_TEXT_SIZE bytes, zero-trailed unless MAX_TEXT_SIZE long)
 * 
 * Note that the box (left, top, width, height) is not stored,
 * because it's irrelevant for a batch run.
 * The box table is stored together with the prototype image.
 */


static void load_record(LibraryRecord *lr, FILE *f)
{
    int i;
    lr->pattern = load_pattern(f);
    lr->disabled = 0;
    lr->radius = read_int32(f);
    for (i = 0; i < MAX_TEXT_SIZE; i++)
        if (!(lr->text[i] = fgetc(f))) break;
}

static void save_record(LibraryRecord *lr, FILE *f)
{
    int i;
    save_pattern(lr->pattern, f);
    write_int32(lr->radius, f);
    for (i = 0; i < MAX_TEXT_SIZE; i++)
    {
        fputc(lr->text[i], f);
        if (!lr->text[i]) break;
    }
}

// _______________________   loading/saving shelves   _________________________


static int load_shelf(Library l, FILE *f)
{
    int proto_size = read_int32(f);
    int i;
    Shelf *s;
    if (feof(f)) return 0;
    if (proto_size <= 0)
    {
        fprintf(stderr,
                "The library seems corrupted or has original images missing.\n"
                "This is so strange that I quit.\n");
        exit(1);
    }
    s = library_append_shelf(l);
    s->offset_in_file = ftell(f);
    fseek(f, proto_size, SEEK_CUR);
    
    s->allocated = s->count = read_int32(f);
    s->pixels = NULL;
    s->records = MALLOC(LibraryRecord, s->count);
    
    for (i = 0; i < s->count; i++)
        load_record(&s->records[i], f);

    return 1;
}


static void shelf_load_prototype(Shelf *s, FILE *f)
{
    int i;
    rle_decode_FILE(f, &s->pixels, &s->width, &s->height);
    s->ownership = 1;
    for (i = 0; i < s->count; i++)
    {
        s->records[i].left   = read_int32(f);
        s->records[i].top    = read_int32(f);
        s->records[i].width  = read_int32(f);
        s->records[i].height = read_int32(f);
    }
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
    long pos1, pos2;
    int i;
    pos1 = ftell(f);
    write_int32(0, f); // to be overwritten later
    shelf_save_prototype(s, f);
    pos2 = ftell(f);
    fseek(f, pos1, SEEK_SET);
    write_int32(pos2 - pos1 - 4 /* without the 0 we've written */, f);
    fseek(f, pos2, SEEK_SET);    
    write_int32(s->count, f);
    for (i = 0; i < s->count; i++)
        save_record(&s->records[i], f);    
}


static void shelf_init(Shelf *s)
    LIST_CREATE(LibraryRecord, s->records, s->count, s->allocated, 8)
    
LibraryRecord *shelf_append(Shelf *s)
    LIST_APPEND(LibraryRecord, s->records, s->count, s->allocated)


Shelf *shelf_create(Library l)
{
    Shelf *s = library_append_shelf(l);
    shelf_init(s);
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
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror(path);
        exit(1);
    }
    
    l->file = f;
    while (1)
        if (!load_shelf(l, f)) break;
    
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
    fclose(l->file);
    l->file = NULL;
}


void library_discard_prototypes(Library l)
{
    fclose(l->file);
    l->file = NULL;
}


void library_write(Library l, const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;
    assert(!l->file);
    
    if (!f)
    {
        perror(path);
        exit(1);
    }
    
    for (i = 0; i < l->count; i++)
        save_shelf(&l->shelves[i], f);

    fclose(f);
}


int library_shelves_count(Library l)
{
    return l->count;
}


Shelf *library_get_shelf(Library l, int i)
{
    return &l->shelves[i];
}
