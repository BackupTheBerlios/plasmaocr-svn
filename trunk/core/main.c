#if 0
gcc -DNTESTING -Wall -g -I../core main.c -o coldplasma ../core/?*.c
exit
#endif


#include "common.h"
#include "core.h"
#include "bitmaps.h"
#include "pnm.h"
#include <unistd.h>
#include <string.h>


#define TAG_BEGIN  '$'
#define TAG_CANCEL '$'
#define TAG_END    '$'



static void usage(void)
{
    /* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */
    fprintf(stderr, "invalid usage\n");
    exit(1);
}


typedef struct
{
    char *input_path;
    char *job_file_path;
    char *out_library_path;
    int colored_output;
    int just_one_letter;
    int just_one_word;
    int append;
    Core core;
    unsigned char **pixels;
    int width, height;
    char *ground_truth;
} Job;

static void init_job(Job *job)
{
    job->input_path = NULL;
    job->job_file_path = NULL;
    job->out_library_path = NULL;
    job->colored_output = isatty(1);
    job->pixels = NULL;
    job->just_one_word = job->just_one_letter = 0;
    job->ground_truth = NULL;
    job->append = 0;
}

static void load_image(Job *job)
{
    int pnm_type;
    if (!job->input_path)
    {
        fprintf(stderr, "You must specify a path to input image\n");
        usage();
    }
    pnm_type = load_pnm(job->input_path, &job->pixels, &job->width, &job->height);
    if (pnm_type != PBM)
    {
        fprintf(stderr, "The image is not a PBM file\n");
        exit(1);
    }
    
}

static void color_print_recognized_letter(RecognizedLetter *l)
{
    switch(l->color)
    {
        case CC_RED:
            printf("\x1B[31m_\x1B[30m");
            break;
        case CC_YELLOW:
            printf("\x1B[31m%s\x1B[30m", l->text);
            break;
        case CC_GREEN: case CC_BLUE:
            printf("%s", l->text);
            break;
    }
}

static void process_word(Job *job, int x, int y, int w, int h)
{
    unsigned char **window = subbitmap(job->pixels, x, y, h);
    RecognizedWord *rw = recognize_word(job->core, window, w, h, 0);
    if (job->colored_output)
    {
        int i;
        for (i = 0; i < rw->count; i++)
            color_print_recognized_letter(rw->letters[i]);
    }
    else
        fputs(rw->text, stdout);
    
    free_recognized_word(rw);
    FREE(window);
}

static void process_letter(Job *job, int x, int y, int w, int h)
{
    unsigned char **window = subbitmap(job->pixels, x, y, h);
    RecognizedLetter *rl = recognize_letter(job->core, window, w, h, 0);
    if (job->colored_output)
        color_print_recognized_letter(rl);
    else if (rl->text)
        fputs(rl->text, stdout);
    else
        putchar('_');

    free_recognized_letter(rl);
    FREE(window);
}

static void skip_to_end_of_tag(FILE *pjf)
{
    int c;
    while ((c = fgetc(pjf)) != TAG_END)
    {
        if (c == EOF)
        {
            fprintf(stderr, "unclosed tag in the job file\n");
            exit(1);
        }
    }
}

static void process_tag(Job *job, FILE *pjf)
{
    int x, y, w, h;
    char tag[11];
    fscanf(pjf, "%10s", tag);
    if (!strcmp(tag, "word"))
    {
        fscanf(pjf, "%d %d %d %d", &x, &y, &w, &h);
        process_word(job, x, y, w, h);
        skip_to_end_of_tag(pjf);
    }
    else if (!strcmp(tag, "letter"))
    {
        fscanf(pjf, "%d %d %d %d", &x, &y, &w, &h);
        process_letter(job, x, y, w, h);
        skip_to_end_of_tag(pjf);
    }
    else
    {
        fprintf(stderr, "unknown PJF tag: %s\n", tag);
    }
}

static void go(Job *job, FILE *pjf)
{
    int c;
    while ((c = fgetc(pjf)) != EOF)
    {
        if (c != TAG_BEGIN)
            putchar(c);
        else
        {
            c = fgetc(pjf);
            if (c == TAG_CANCEL)
                putchar(c);
            else
            {
                ungetc(c, pjf);
                process_tag(job, pjf);
            }
        }
    }
}

#ifndef TESTING

int main(int argc, char **argv)
{
    Job job;
    int i;
    FILE *pjf;

    init_job(&job);
    job.core = create_core();
    
    for (i = 1; i < argc; i++)
    {
        char *opt = argv[i];
        char *arg = argv[i + 1];
        if (opt[0] == '-' && strcmp(opt, "-"))
        {
            if (!strcmp(opt, "-L") || !strcmp(opt, "--letter"))
            {
                job.just_one_letter = 1;
                job.just_one_word = 0;
            }
            if (!strcmp(opt, "-t") || !strcmp(opt, "--truth"))
            {
                i++; if (!arg) usage();
                job.just_one_letter = 1;
                job.just_one_word = 0;
                job.ground_truth = arg;
            }
            if (!strcmp(opt, "-a") || !strcmp(opt, "--append"))
            {
                job.append = 1;
            }
            else if (!strcmp(opt, "-W") || !strcmp(opt, "--word"))
            {
                job.just_one_letter = 0;
                job.just_one_word = 1;
            }
            else if (!strcmp(opt, "-l") || !strcmp(opt, "--lib"))
            {
                Library l;
                i++; if (!arg) usage();
                l = library_open(arg);
                library_discard_prototypes(l);
                add_to_core(job.core, l);
            }
            else if (!strcmp(opt, "-p") || !strcmp(opt, "-j") || !strcmp(opt, "--pjf"))
            {
                i++; if (!arg) usage();
                job.job_file_path = arg;
            }
            else if (!strcmp(opt, "-i") || !strcmp(opt, "--in"))
            {
                i++; if (!arg) usage();
                job.input_path = arg;                
            }
            else if (!strcmp(opt, "-o") || !strcmp(opt, "--out"))
            {
                i++; if (!arg) usage();
                set_core_orange_policy(job.core, 1);
                job.out_library_path = arg;
            }
            else if (!strcmp(opt, "-c") || !strcmp(opt, "--color"))
            {
                job.colored_output = 1;
            }
            else if (!strcmp(opt, "-n") || !strcmp(opt, "--nocolor"))
            {
                job.colored_output = 0;
            }
        }
    }


    load_image(&job);

    if (job.just_one_letter)
    {
        process_letter(&job, 0, 0, job.width, job.height);
        putchar('\n');
        if (job.ground_truth && job.out_library_path)
        {
            Library l = get_core_orange_library(job.core);
            if (library_shelves_count(l))
            {
                Shelf *s = library_get_shelf(l, 0);
                if (s->count)
                {
                    strncpy(s->records[0].text, job.ground_truth, MAX_TEXT_SIZE);
                }
            }
        }
    }
    else if (job.just_one_word)
    {
        process_word(&job, 0, 0, job.width, job.height);
        putchar('\n');
    }
    else 
    {
        if (!job.job_file_path)
        {
            fprintf(stderr, "You must specify a pjf file (or either -L or -W).\n");
            usage();
        }
        pjf = fopen(job.job_file_path, "r");
        if (!pjf)
        {
            perror(job.job_file_path);
            exit(1);
        }
        go(&job, pjf);
    }

    if (job.out_library_path)
        library_save(get_core_orange_library(job.core), job.out_library_path, job.append);

    free_bitmap(job.pixels);
    free_core(job.core);
    return 0;
}

#endif
