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


#include "array.h"
#include "testing.h"

/** \file array.cpp
 * \brief Some tests for `\ref array.h'.
 */

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
