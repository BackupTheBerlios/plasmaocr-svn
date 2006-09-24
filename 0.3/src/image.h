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


#ifndef PLASMA_OCR_IMAGE_H
#define PLASMA_OCR_IMAGE_H

#include "common.h"
#include <assert.h>

PLASMA_BEGIN;

/** \file image.h
 * \brief Contains image-related classes:
 *      Rect, FloatingImage, Image, Bitmap, Graymap and Pixmap.
 *
 * \todo comment
 */


#ifndef NDEBUG
    /// A boundary-checking wrapper for (byte *).
    class Row
    {
        int min_x, max_x;
        byte *data;
        
        friend class FloatingImage;
        
        public:
            byte &operator[] (int x)
            {
                assert(x >= min_x);
                assert(x <= max_x);
                return data[x];
            }
    };
#else
    typedef byte *Row;
#endif


/** \brief A rectangle.
 *
 * Empty rectangles are forbidden.
 */

class Rect
{
    int m_left, m_top, m_width, m_height;

public:
    
    inline Rect(int left, int top, int width, int height):
        m_left(left), m_top(top), m_width(width), m_height(height) 
    {
        assert(width > 0);
        assert(height > 0);
    }
    
    /// Return the minimal x coordinate of pixels in the rectangle.
    inline int x_min()  { return m_left; }
    
    /// Return the inter-pixel coordinate of the left edge.
    inline int left()   { return m_left; }
    
    /// Return the minimal y coordinate of pixels in the rectangle.
    inline int y_min()  { return m_top;  }

    /// Return the inter-pixel coordinate of the top edge.
    inline int top()    { return m_top;  }

    /// Return the inter-pixel coordinate of the right edge.
    inline int right()  { return m_left + m_width; }
    
    /// Return the maximal x coordinate of pixels in the rectangle.
    inline int x_max()  { return m_left + m_width - 1; }

    /// Return the inter-pixel coordinate of the bottom edge.
    inline int bottom() { return m_top + m_height;     }

    /// Return the maximal y coordinate of pixels in the rectangle.
    inline int y_max()  { return m_top + m_height - 1; }

    /// Return true if the point (x,y) is inside the rectangle.
    inline bool contains(int x, int y)
    {
        return x >= x_min() && x <= x_max()
            && y >= y_min() && y <= y_max();
    }

    /// Return true if the given rectangle is a subset of this one.
    inline bool contains(const Rect &r)
    {
        return r.left() >= left() && r.right() <= right()
            && r.top()  >= top()  && r.bottom() <= bottom();
    }
};


/** \brief A bitmap, graymap or pixmap.
 *
 * An Image represents a 2-dimensional buffer,
 * not necessarily owned by the Image object.
 *
 * The image rectangle may be everywhere, not necessarily with
 * (0, 0) as the left upper corner.
 *
 * In bitmap mode, the Image doesn't compress or pack its bits in any
 * way, thus using one byte per pixel. Nonzero values correspond to
 * black, zero to white. If ZERO_OR_ONE flag is set, the buffer must
 * contain only 0 and 1 values.
 *
 * In pixmap mode, every row contains (3 x width) bytes, ordered RGBRGBRGB...
 *
 * Y increases from top to bottom.
 */
class FloatingImage : public Rect
{   
    private:
        byte **pixels;
        int flags;
    
    public:
    
        enum Type
        {
            BITMAP,
            GRAYMAP,
            PIXMAP
        };

        virtual Type type() = 0;
        
        
        enum
        {
            ONE_PIECE       =   01,
            FREE_PIXELS     =   02,
            FREE_POINTERS   =   04,
            DELETE_PIXELS   =  010,
            DELETE_POINTERS =  020,
            ZERO_OR_ONE     =  040
        };

    /* Allocate an image with the specified parameters.
     * If pixels == NULL, then flags ONE_PIECE, DELETE_PIXELS and
     * DELETE_POINTERS are set regardless of `flags' argument.
     * If the type == PIXMAP, the buffer will be 3 times larger.
     */
    FloatingImage(const Rect &, int row_size, int flags = 0);
    FloatingImage(const Rect &, byte **pixels, int flags = 0);
    
    virtual ~FloatingImage();
    
    FloatingImage &operator =(const FloatingImage &);

    void invert();
    void clear();
    
    
    /** \brief The number of bytes occupied by a row.
     * Equal to width for bitmaps and graymaps and 3 times larger for pixmaps.
     */
    virtual int row_size() = 0;
    
    bool operator ==(const FloatingImage &);
    
#   ifndef NDEBUG
        void assert_ok_parameters();
#   endif
};

class Image : public FloatingImage
{
};

class Bitmap : public Image
{
    void force_0_or_1();
    virtual Type type();
};

class Graymap : public Image
{
    virtual Type type();
};

class Pixmap : public Image
{
    virtual Type type();
};


PLASMA_END;

#endif
