#ifndef PLASMA_OCR_COMMON_H
#define PLASMA_OCR_COMMON_H


/** \file common.h
 * \brief A portability header
 * 
 * This file is meant to be included in all the headers in this project.
 * It contains namespace-dancing macros and a bunch of typedefs.
 *
 * \todo Make the typedefs more automatic (autoconf?)
 */

/** \namespace plasma
 * 
 * That's the namespace in which all the stuff resides unless NO_NAMESPACES
 * is defined. Macros PLASMA_BEGIN, PLASMA_END and PLASMA_MAIN should do
 * the namespace handling. Unfortunately, these macros also prevent Doxygen
 * from understanding what's going on.
 */

#ifndef NO_NAMESPACES

#   define PLASMA_BEGIN namespace plasma {
#   define PLASMA_END }
#   define PLASMA_MAIN \
    main(int argc, char *argv[]); \
    PLASMA_END; \
    int main(int argc, char *argv[]) \
    { \
        return plasma::main(argc, argv); \
    } \
    PLASMA_BEGIN; \
    int main

#else

#   define PLASMA_BEGIN
#   define PLASMA_END
#   define PLASMA_MAIN main

#endif


PLASMA_BEGIN;

#ifndef HAVE_TYPES
typedef signed char int8;
typedef unsigned char byte;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;
#endif

PLASMA_END;


#endif
