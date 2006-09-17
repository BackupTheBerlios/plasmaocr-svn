#include "array.h"
#include "testing.h"

PLASMA_BEGIN;

#ifndef NDEBUG

/// Some simple pokings into the Array class.
TEST(arrays)
{
    Array<int> a(0, 1);
    assert(a.count() == 0);
    for (int i = 0; i < 20; i++)
    {
        a.append(i * i);
        assert(a.count() == i + 1);
        assert(a[i] == i * i);
        a[i] *= i;
        assert(a[i] == i * i * i);
    }
    Array<int> b(a);
    for (int i = 0; i < 20; i++)
        assert(b[i] == i * i * i);
    Array<int> c(20);
    c = b;
    for (int i = 0; i < 20; i++)
        assert(b[i] == i * i * i);

}

#endif

PLASMA_END;
