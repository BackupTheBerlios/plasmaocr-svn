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


/** \file array.h
 * \brief Dynamically growing array.
 * 
 * This file contains only the Array template.
 */


#ifndef PLASMA_OCR_ARRAY_H
#define PLASMA_OCR_ARRAY_H


#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

PLASMA_BEGIN;

/// A dynamically growing array.
/**
 * Array uses malloc()/free() heap.
 * Use it only with objects that don't require construction/destruction.
 *
 * All methods of this class are inlined.
 * 
 * \warning Never store pointers to array's items outside the array,
 * because the array might relocate its items.
 */

template<class T> class Array
{
    /// The buffer.
    T *buffer;

    /// How many items we actually have.
    int size;

    /// The size of our buffer, in items.
    int allocated;
    
    public:
        
        /// Create an array with the specified initial size and capacity.
        /**
         * The items are not initialized even if the initial size is nonzero.
         * The initial capacity must be no less than the initial size.
         */
        inline Array(int initial_count = 0, int initial_capacity = 0)
        {
            assert(initial_count >= 0);
            assert(!initial_capacity || initial_capacity >= initial_count);
            if (!initial_capacity)
                initial_capacity = 8;   // default initial capacity
            if (initial_capacity < initial_count)
                initial_capacity = initial_count;
            buffer = (T *) malloc(sizeof(T) * initial_capacity);
            size = initial_count;
            allocated = initial_capacity;
        }

        
        /// Destroy the array, freeing its buffer.
        /** The items are not deinitialized.
         */
        inline ~Array()
        {
            free(buffer);
        }

        /// Apply the function to all items.
        inline void apply(void (*func)(T &))
        {
            for (int i = 0; i < size; i++)
                (*func)(buffer[i]);
        }


        /// Set the buffer capacity to the designated value.
        /** New capacity must be positive and no less than item count.
         */
        inline void set_capacity(int capacity)
        {
            assert(capacity > 0);
            assert(capacity >= size);
            allocated = capacity;
            buffer = (T *) realloc(buffer, sizeof(T) * allocated);
        }
        
        /// Call set_capacity(), if the new capacity is greater.
        inline void ensure_capacity(int capacity)
        {
            if (allocated < capacity)
                set_capacity(capacity);
        }
        
        
        /// Append a new item to the end of the array and return a pointer to it.
        /** Increases count by 1. Relocates the buffer if necessary.
         */
        inline T *append()
        {
            if (size == allocated)
                set_capacity(allocated * 2);
            return &buffer[size++];
        }


        /// Append the given item to the end of the array.
        /** Uses assignment operator. */
        inline void append(const T &new_item)
        {
            *append() = new_item;
        }
        
        
        /// Return the number of items.
        inline int count()
        {
            return size;
        }

        
        /// Access an item. Checks bounds by assert().
        inline T & operator[] (int index)
        {
            assert(index >= 0 && index < size);
            return buffer[index];
        }

        
        /// Sets the buffer's size to the minimal possible value.
        inline void trim()
        {
            set_capacity(size ? size : 1);
        }


        /// Copy constructor.
        inline Array(const Array<T> &a)
        {
            size = a.size;
            allocated = a.allocated;
            buffer = (T *) malloc(sizeof(T) * allocated);
            memcpy(buffer, a.buffer, sizeof(T) * size);
        }
        
        
        /// Copy operator.
        inline Array<T> &operator =(const Array<T> &a)
        {
            ensure_capacity(a.allocated);
            size = a.size;
            memcpy(buffer, a.buffer, sizeof(T) * size);
            return *this;
        }
};

PLASMA_END;


#endif
