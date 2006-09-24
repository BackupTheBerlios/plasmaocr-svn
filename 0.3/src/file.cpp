#include "file.h"
#include "testing.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


PLASMA_BEGIN;

File::File(const char *path, const char *mode)
    : stream(NULL), ownership(true)
{
    if (!strcmp(mode, "tmp"))
    {
        stream = tmpfile();
        return;
    }
            
    assert(path);
    if (!strcmp(path, "-"))
    {
        if (strchr(mode, '+'))
        {
            fprintf(stderr, "cannot open '-' with mode %s\n", mode);
            exit(1);
        }
        
        if (strchr(mode, 'r'))
            stream = stdin;
        else
            stream = stdout;
        ownership = false;
        return;
    }
    
    if (*mode == '+' || !strcmp(mode, "b+"))
    {
        char buf[5];
        sprintf(buf, "r%2s", mode);
        stream = fopen(path, buf);
        if (!stream)
        {
            sprintf(buf, "w%2s", mode);
            stream = fopen(path, buf);
        }
    }
    else    
        stream = fopen(path, mode);
    
    if (!stream)
    {
        perror(path);
        exit(1);
    }
}

File::~File()
{
    if (ownership)
        fclose(stream);
}

int File::printf(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    int result = vfprintf(stream, format, v);
    va_end(v);
    return result;
}

int File::scanf(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    int result = vfscanf(stream, format, v);
    va_end(v);
    return result;
}


#define MAGIC16 45678

TEST(files)
{
    File *f = new File("../test/.foo", "w");
    f->printf("%d", 57);
    delete f;
    
    f = new File("../test/.foo", "+");
    int i;
    f->scanf("%d", &i);
    assert(i == 57);
    assert(f->eof());
    f->seek_set(0);
    f->write_uint16(MAGIC16);
    f->seek_set(0);
    assert(f->read_uint16() == MAGIC16);
    
    delete f;
}

PLASMA_END;
