#if 0
    gcc -o coldplasma -g -I../my/minidjvu-0.5 -I../core -Wall ../core/bitmaps.c ../core/linewise.c pnm.c shiftcut.c main.c layout.c code2tex.c -Wl,--rpath -Wl,/usr/local/lib -lminidjvu
    exit    
#endif


#include "code2tex.h"
#include "layout.h"
#include "pnm.h"
#include "shiftcut.h"
#include "memory.h"
#include "bitmaps.h"
#include <assert.h>


#define SIZE_COEF 1.3

    
typedef struct
{
    int code;
    int width;
    int height;
    Fingerprint fingerprint;
} Sample;
    

typedef struct
{
    unsigned char **pixels; // the whole page
    int width;
    int height;
    int is_gray;  // otherwise black and white
    Sample *samples;
    int sample_count;
    TransitionTable table;
} Job;


static int read_uint16(FILE *f)
{
    int i = fgetc(f) << 8;
    return i | fgetc(f);
}


static int read_int32(FILE *f)
{
    int i = read_uint16(f) << 16;
    return i | read_uint16(f);
}


static void read_sample(FILE *f, Sample *s)
{
    s->code   = read_uint16(f);
    s->width  = read_uint16(f);
    s->height = read_uint16(f);
    fread(s->fingerprint, 1, sizeof(s->fingerprint), f);
}


static void read_library(const char *path, Job *job)
{
    int i;
    FILE *f = fopen(path, "rb");
    
    if (!f)
    {
        perror(path);
        exit(1);
    }

    job->sample_count = read_int32(f);
    job->samples = MALLOC(Sample, job->sample_count);
    for (i = 0; i < job->sample_count; i++)
        read_sample(f, &job->samples[i]);
    
    fclose(f);
}


static void destroy_job(Job *job)
{
    free_bitmap(job->pixels);
    transition_table_destroy(job->table);
    FREE(job->samples);
}


static Sample *find_nearest_sample(Sample *samples, int nsamples, int w, int h, Fingerprint f)
{
    Sample *best = NULL;
    long best_dist = 0x7FFFFFFFL;
    int i;

    for (i = 0; i < nsamples; i++)
    {
        int a = samples[i].width  * h;
        int b = samples[i].height * w;
        if (a > SIZE_COEF * b || b > SIZE_COEF * a)
            continue;

        long dist = fingerprint_distance_squared(samples[i].fingerprint, f);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = &samples[i];
        }
    }
 
    return best;
}


static Sample *find_best_match(Job *job, Box *box)
{
    unsigned char **pointers = MALLOC(unsigned char *, box->h);
    int i;
    Sample *result;
    Fingerprint f;

    for (i = 0; i < box->h; i++)
        pointers[i] = job->pixels[i + box->y] + box->x;

    if (job->is_gray)
        get_fingerprint_gray (pointers, box->w, box->h, &f);
    else
        get_fingerprint_bw   (pointers, box->w, box->h, &f);
 
    result = find_nearest_sample(job->samples, job->sample_count, box->w, box->h, f);

    FREE(pointers);
    return result;
}


static void dump_char(Job *job, List *l)
{
    Box *box;
    assert(l->type == LIST_LEAF);
    box = l->box;
    if (box->is_space)
        putchar(' ');
    else
    {
        printf("%s", transition_table_lookup(job->table,
                        find_best_match(job, box)->code));
    }
}

static void dump_line(Job *job, List *l)
{
    int i;
    assert(l->type == LIST_LINE);
    for (i = 0; i < l->count; i++)
        dump_char(job, &l->items[i]);
    putchar('\n');
}


static void dump_block(Job *job, List *l)
{
    int i;
    assert(l->type == LIST_BLOCK);
    for (i = 0; i < l->count; i++)
        dump_line(job, &l->items[i]);
}


static void dump_page(Job *job, List *l)
{
    int i;
    /*print_bitmap(job->pixels, job->width, job->height);
    exit(1);*/
    assert(l->type == LIST_PAGE);
    for (i = 0; i < l->count; i++)
        dump_block(job, &l->items[i]);
}


int main(int argc, char **argv)
{
    char *input_path, *charlib_path, *codetable_path, *ocrad_result_path;
    Job job;
    List list;
    BoxList *boxlist;
    int pnm_type;
    
    if (argc != 5)
    {
        fprintf(stderr, "4 arguments required: input, charlib, codetable and Ocrad -x.\n");
        fprintf(stderr, "Or use (or try to fix) the `plasma' shell script.\n");
        exit(2);
    }

    input_path = argv[1];
    charlib_path = argv[2];
    codetable_path = argv[3];
    ocrad_result_path= argv[4];
    
    read_library(charlib_path, &job);
    job.table = transition_table_load(codetable_path);

    FILE *input = fopen(input_path, "r");
    if (!input)
    {
        perror(input_path);
        exit(1);
    }
    pnm_type = load_pnm(input, &job.pixels, &job.width, &job.height);
    fclose(input);
    
    switch(pnm_type)
    {
        case PBM:
            job.is_gray = 0;
        break;
        case PGM:
            job.is_gray = 1;
        break;
        default:
            fprintf(stderr, "PPM not supported, use ppmtopgm on it\n");
            exit(1);
    }

    FILE *ocrad = fopen(ocrad_result_path, "r");
    if (!ocrad)
    {
        perror(ocrad_result_path);
        exit(1);
    }
    boxlist = parse_ocrad(&list, ocrad);
    fclose(ocrad);
    
    dump_page(&job, &list);
    
    list_destroy_children(&list);
    box_list_destroy(boxlist);
    destroy_job(&job);
    
    return 0;
}
