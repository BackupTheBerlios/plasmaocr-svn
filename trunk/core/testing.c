#include "common.h"

#ifdef TESTING

#include <stdio.h>
#include <unistd.h>
#include "bitmaps.h"
#include "fft.h"
#include "chaincode.h"
#include "editdist.h"


/* This test is useless - its success is guaranteed by the language standard.
 * In fact it's our testing framework we're checking.
 */
static void test_paranoia()
{
    assert(1);
    assert(sizeof(unsigned char) == 1);
}

static TestFunction basic_tests[] = {test_paranoia, NULL};
static TestSuite basic_suite = {"Basic", NULL, NULL, basic_tests};


/* ------------  THE LIST OF TEST SUITES --------------*/

static TestSuite *suites[] = {&basic_suite,
                              &bitmaps_suite,
                              &chaincode_suite,
                              &fft_suite,
                              &editdist_suite,
                              NULL};


static int count_nonzero_pointers(void **array)
{
    int i = 0;
    while (*array++) i++;
    return i;
}


static int get_total_number_of_tests(TestSuite **suites)
{
    int i;
    int result = 0;
    int nsuites = count_nonzero_pointers((void **) suites);
    for (i = 0; i < nsuites; i++)
    {
        result += count_nonzero_pointers((void **) (suites[i]->tests));
    }
    return result;
}


static void output_prepare(int total_number)
{
    int i;
    putchar('[');
    for (i = 0; i < total_number; i++)
        putchar('.');
    putchar(']');
    fflush(stdout);
}

static void output_show(int position, int total_number)
{
    int i;
    for (i = 0; i < total_number - position + 2; i++)
        putchar('\b');    
    putchar('#');
    for (i = 0; i < total_number - position; i++)
        putchar('.');    
    putchar(']');
    fflush(stdout);
}

static void output_finish()
{
    putchar('\n');
}


int main()
{
    int nsuites = count_nonzero_pointers((void **) suites);
    int total_number_of_tests = get_total_number_of_tests(suites);
    int i;
    int counter = 0;

    printf("Tests: %d\n", total_number_of_tests);

    output_prepare(total_number_of_tests);
    for (i = 0; i < nsuites; i++)
    {
        int test = 0;
        if (suites[i]->init_func)
            suites[i]->init_func();

        while (suites[i]->tests[test])
        {
            suites[i]->tests[test]();
            output_show(++counter, total_number_of_tests);
            test++;
        }

        if (suites[i]->destroy_func)
            suites[i]->destroy_func();
    }
    output_finish();
    
    return 0;
}


#endif


/* Just a little export symbol to keep the compiler happy. */
void dummy_function_testing()
{
}
