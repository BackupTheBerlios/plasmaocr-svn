#ifndef TESTING_H
#define TESTING_H
#ifdef TESTING

#ifdef NDEBUG
#   error TESTING and NDEBUG make no sense together!
#endif


#include <assert.h>

typedef void (*TestFunction)(void);

typedef struct
{
    char *name;
    void (*init_func)(void);
    void (*destroy_func)(void);
    TestFunction *tests;
} TestSuite;

#else

/* only to keep the compiler happy (it may complain on empty files) */
void dummy_function_testing(void);

#endif /* TESTING */
#endif /* not TESTING_H */
