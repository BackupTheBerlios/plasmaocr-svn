/* Plasma OCR - an OCR engine
 *
 * bitmaps.h - some routines that work with bitmaps
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


#ifndef PLASMA_OCR_BITMAP_H
#define PLASMA_OCR_BITMAP_H


#include "common.h"

FUNCTIONS_BEGIN

/* Just allocate a w * h array. */
unsigned char **allocate_bitmap(int w, int h);
void free_bitmap(unsigned char **);
void assign_bitmap(unsigned char **dst, unsigned char **src, int w, int h);
void assign_bitmap_with_offsets(unsigned char **dst, unsigned char **src, int w, int h, int dst_offset, int src_offset);
unsigned char **copy_bitmap(unsigned char **, int w, int h);


/* Allocate a w * h bitmap with margins of 1 pixels at each side.
 * Indices into such a bitmap range from -1 to w or h.
 * Free the allocated bitmap with free_bitmap_with_margins().
 */
unsigned char **allocate_bitmap_with_margins(int w, int h);
void free_bitmap_with_margins(unsigned char **);

/* Allocate bitmap with margins and clear those margins. */
unsigned char **allocate_bitmap_with_white_margins(int w, int h);



void print_bitmap(unsigned char **, int w, int h);


/* Allocate a w * h bitmap with margins of 1 pixels at each side.
 * Copy `pixels' there and clear the margins.
 */
unsigned char **provide_margins(unsigned char **, int w, int h, int make_it_0_or_1);


void make_bitmap_0_or_1(unsigned char **, int w, int h);


void invert_bitmap(unsigned char **, int w, int h, int first_make_it_0_or_1);

void clear_bitmap(unsigned char **pixels, int w, int h);

int bitmaps_equal(unsigned char **p1, unsigned char **p2, int w, int h);

/* Clear pixels that have exactly 1 of 4 (or 8) neighbors.
 *
 * `pixels' must have margins.
 *
 * `result' cannot coincide with `pixels'.
 */
void strip_endpoints_4(unsigned char **result, unsigned char **pixels, int w, int h);
void strip_endpoints_8(unsigned char **result, unsigned char **pixels, int w, int h);

int find_mass(unsigned char **pixels, int w, int h);
void find_mass_center(unsigned char **pixels, int w, int h, int *m, int *cx, int *cy, int quant);


/* Tighten a rectangle to a bbox of its contents.
 * Returns nonzero if the image is non-empty.
 * (if it's empty, then b_w and b_h are set to 1)
 */
int tighten_to_bbox(unsigned char **pixels, int w,
                    int *b_x, int *b_y, int *b_w, int *b_h);

/* Find a bounding box. Returns nonzero if the image is non-empty.
 */
int find_bbox(unsigned char **pixels, int w, int h,
              int *b_x, int *b_y, int *b_w, int *b_h);


/* Returns true if *bbox should be FREE()'d afterwards.
 * In case the bbox is empty, return 1x1 white pixel.
 */
int get_bbox_window(unsigned char **pixels, int w, int h,
                    unsigned char ***bbox, int *b_w, int *b_h);

unsigned char **subbitmap(unsigned char **pixels, int x, int y, int h);

unsigned char **simple_noise(int w, int h);

FUNCTIONS_END

#ifdef TESTING
extern TestSuite bitmaps_suite;
#endif

#endif
