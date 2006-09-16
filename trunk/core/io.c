#include "common.h"
#include "io.h"
#include <string.h>


void write_int32(int i, FILE *f)
{
    fputc(i >> 24, f);
    fputc(i >> 16, f);
    fputc(i >> 8, f);
    fputc(i, f);    
}


int read_int32(FILE *f)
{
    int i = fgetc(f);
    i = (i << 8) | fgetc(f);
    i = (i << 8) | fgetc(f);
    i = (i << 8) | fgetc(f);
    return i;
}


FILE *checked_fopen(const char *path, const char *mode)
{
    FILE *f;
    if (!strcmp(path, "-"))
    {
        if (strchr(mode, '+'))
        {
            fprintf(stderr, "cannot open '-' with mode %s\n", mode);
            exit(1);
        }
        
        if (strchr(mode, 'r'))
            return stdin;
        else
            return stdout;
    }
    
    f = fopen(path, mode);
    if (!f)
    {
        perror(path);
        exit(1);
    }

    return f;
}


void checked_fclose(FILE *f)
{
    if (f != stdin && f != stdout)
        fclose(f);
}


#ifdef TESTING

struct FilePairStruct
{
    FILE *in;
    FILE *out;
};


#ifdef NO_PIPES


FilePair file_pair_open();
{
    FilePair *fp = MALLOC1(struct FilePairStruct);
    if (!(*in = *out = tmpfile()))
    {
        perror("unable to create temporary file");
        exit(1);
    }
}

FILE *file_pair_read(FilePair fp)
{
    rewind(fp->in);
    return fp->in;
}

FILE *file_pair_write(FilePair fp)
{
    return file_pair_read(fp);
}

void file_pair_close(FilePair fp)
{
    fclose(fp->in);
    FREE1(fp);
}

#else /* use pipes */

#include <unistd.h>

/* for the "maniac" compiling mode */
FILE *fdopen(int fildes, const char *mode);


FilePair file_pair_open()
{
    FilePair fp = MALLOC1(struct FilePairStruct);
    int filedes[2];
    if (pipe(filedes))
    {
        perror("failed to open pipe");
        exit(1);
    }
    fp->in = fdopen(filedes[0], "rb");
    fp->out = fdopen(filedes[1], "wb");
    return fp;
}


FILE *file_pair_read(FilePair fp)
{
    fflush(fp->out);
    return fp->in;
}

FILE *file_pair_write(FilePair fp)
{
    return fp->out;
}

void file_pair_close(FilePair fp)
{
    fclose(fp->in);
    fclose(fp->out);
    FREE1(fp);
}

#endif /* NO_PIPES */


static void test_pipes(void)
{
    unsigned char buf1[100], buf2[100];
    FilePair fp = file_pair_open();
    FILE *f;
    int i;
    
    srand(57);
    for (i = 0; i < sizeof(buf1); i++)
        buf1[i] = (unsigned char) rand();

    f = file_pair_write(fp);
    fwrite(buf1, 1, sizeof(buf1), f);
    f = file_pair_read(fp);
    fread(buf2, 1, sizeof(buf1), f);    

    assert(!memcmp(buf1, buf2, sizeof(buf1)));
    
    file_pair_close(fp);
}


static TestFunction tests[] = {
    test_pipes,
    NULL
};

TestSuite io_suite = {"io", NULL, NULL, tests};


#endif /* TESTING */
