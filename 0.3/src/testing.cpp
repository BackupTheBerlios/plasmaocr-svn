#include "array.h"
#include "testing.h"
#include <stdio.h>
#include <stdlib.h>

PLASMA_BEGIN;

/** \brief Test sizes of primitive typedefs.
 */
TEST(sizeofs)
{
    if (sizeof(int8) != 1
     || sizeof(byte) != 1
     || sizeof(uint8) != 1
     || sizeof(int16) != 2
     || sizeof(uint16) != 2
     || sizeof(int32) != 4
     || sizeof(uint32) != 4
     || sizeof(int64) != 8
     || sizeof(uint64) != 8)
    {
        fprintf(stderr, "The test of primitive types' sizes failed.\n");
        fprintf(stderr, "Please look into `common.h' and set valid typedefs.\n");
        exit(1);
    }
}


static Array<Test *> tests;

/** \brief Initialization-time backup for `tests'.
 *
 * The registering routines are called from constructors. Therefore,
 * it might happen that the tests array won't be initialized at the moment.
 * So we create a version on the heap, initialize it, and later copy it back.
 */
static Array<Test *> *dyn_tests = NULL;


Test::Test(const char *n, const char *f, int l) : name(n), file(f), line(l)
{
    if (!dyn_tests)
        dyn_tests = new Array<Test *>();
        
    dyn_tests->append(this);
}


static void run_test(Test *&test)
{
    printf("%s (%s:%d)\n", test->name, test->file, test->line);
    test->run();
}

void run_all_tests()
{
    if (dyn_tests)
    {
        tests = *dyn_tests;
        delete dyn_tests;
        dyn_tests = NULL;
    }
    printf("%d tests to execute\n\n", tests.count());
    tests.apply(run_test);
    printf("\n%d tests executed\n", tests.count());
}

PLASMA_END;
