/* Plasma OCR - an OCR engine
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


#ifndef PLASMA_OCR_TESTING_H
#define PLASMA_OCR_TESTING_H


/** \file testing.h
 * \brief simple framework for testing
 *
 * This file provides the TEST macro, run_all_tests() function and
 * the Test class. This should be enough for now.
 */

#include "common.h"
PLASMA_BEGIN;


/** \def TEST
 * \brief A macro to create tests.
 * 
 * Use it like this:
 * \code
 *      #ifndef NDEBUG  // (optionally)
 *      TEST(test_that_life_is_good)
 *      {
 *          assert(power_is_on());
 *          assert(user_is_nearby());
 *          ...
 *      }
 *      #endif
 * \endcode
 *
 * The test will register automatically at startup and will be run by run_all_tests().
 */
#define TEST(TEST_NAME) static struct TestClass_##TEST_NAME: Test \
{ \
    TestClass_##TEST_NAME() : Test(#TEST_NAME, __FILE__, __LINE__) {} \
    virtual void run(); \
} TestObject_##TEST_NAME; \
\
void TestClass_##TEST_NAME::run()


/** \brief An abstract Test
 * (used only internally in testing.cpp and the TEST macro). 
 *
 * The constructor of Test appends the object to a global list.
 * That list will be later used by run_all_tests().
 */
struct Test
{
    const char *name;
    const char *file;
    const int line;
    Test(const char *name, const char *file, int line);
    virtual void run() = 0;
};


/** \brief Run all tests.
 * 
 * This function can, for example, be called in main() like this:
 *
 * \code
 *   #ifdef TESTING
 *      run_all_tests();
 *      exit(0);
 *   #endif
 * \endcode
 */
void run_all_tests();


PLASMA_END;


#endif
