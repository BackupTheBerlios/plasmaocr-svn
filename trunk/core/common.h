/* Plasma OCR - an OCR engine
 *
 * common.h - some common macros
 *
 * Copyright (C) 2006  Ilya Mezhirov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef PLASMA_OCR_COMMON_H
#define PLASMA_OCR_COMMON_H


/*
#ifndef NDEBUG
#   ifndef NTESTING
#       define TESTING
#   endif
#endif
*/

#ifdef TESTING
    #include "testing.h"
#endif



#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif


#ifdef __cplusplus
#   define FUNCTIONS_BEGIN extern "C" {
#   define FUNCTIONS_END }
#else
#   define FUNCTIONS_BEGIN
#   define FUNCTIONS_END
#endif



#include <stdlib.h>


#define MALLOC1(TYPE)           ( (TYPE *) malloc(sizeof(TYPE)) )
#define MALLOC(TYPE, N)         ( (TYPE *) malloc((N) * sizeof(TYPE)) )
#define REALLOC(TYPE, PTR, N)   ( (TYPE *) realloc(PTR, (N) * sizeof(TYPE)) )
#define FREE1(PTR)              free(PTR)
#define FREE(PTR)               free(PTR)


/* Yeah, I know this is ugly and there are C++ templates for that.
 * But this is for C code.
 */
#define LIST_APPEND(TYPE, LIST, COUNT, ALLOCATED) \
{                                                 \
    if ((COUNT) == (ALLOCATED))                   \
    {                                             \
        (ALLOCATED) <<= 1;  /* double the list */ \
        (LIST) = REALLOC(TYPE, LIST, ALLOCATED);  \
    }                                             \
    return &(LIST)[(COUNT)++];                    \
}


#define LIST_CREATE(TYPE, LIST, COUNT, ALLOCATED, START) \
{                                                        \
    (LIST) = MALLOC(TYPE, START);                        \
    (ALLOCATED) = (START);                               \
    (COUNT) = 0;                                         \
}


#endif
