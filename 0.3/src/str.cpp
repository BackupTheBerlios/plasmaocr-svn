#include "str.h"
#include "testing.h"
#include <string.h>

PLASMA_BEGIN;

String::Buffer::Buffer(const char *s) : link_counter(1), str(NULL)
{
    assert(s);

    // Not using strdup here because it's not ANSI
    str = new char[strlen(s) + 1];
    strcpy(str, s);
}

String::Buffer::~Buffer()
{
    assert(link_counter == 0);
    delete [] str;
}

TEST(strings)
{
    String s = "hello";
    String t = s;
    assert(!strcmp(t, "hello"));
    t = "preved";
    assert(!strcmp(s, "hello"));
    assert(!strcmp(t, "preved"));
    t = s;
    assert(!strcmp(t, "hello"));
    String *a = new String("1");
    String *b = new String(*a);
    delete a;
    assert(!strcmp(*b, "1"));
    delete b;
}

PLASMA_END;
