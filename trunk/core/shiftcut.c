/* Plasma OCR - an OCR engine
 *
 * shiftcut.c - compute shift-n-cut fingerprints
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



/* We cut an image horizontally in such a way
 *     that below and above the cut the blackness is roughly the same.
 * Than we cut each of the two pieces vertically in the same fashion.
 * Then horizontally, and so on until we've made sizeof(Fingerprint) cuts.
 * The position of each cut is normalized into 0..255 and stored in the fingerprint.
 */


#include "common.h"
#include "shiftcut.h"
#include <assert.h>
#include <stdio.h>


typedef unsigned char byte;


static int sum_column_gray(byte **pixels, int x, int y1, int y2)
{
    int sum = 0, y;
    for (y = y1; y <= y2; y++) sum += 255 - pixels[y][x]; // XXX
    return sum;
}


static int sum_row_gray(byte *row, int x1, int x2)
{
    int sum = 0, x, n = x2 - x1;
    byte *p = row + x1;
    for (x = 0; x <= n; x++) sum += 255 - p[x]; // XXX
    return sum;
}


static int sum_column_bw(byte **pixels, int x, int y1, int y2)
{
    int sum = 0, y;
    for (y = y1; y <= y2; y++) if (pixels[y][x]) sum++;
    return sum;
}


static int sum_row_bw(byte *row, int x1, int x2)
{
    int sum = 0, x, n = x2 - x1;
    byte *p = row + x1;
    for (x = 0; x <= n; x++) if (p[x]) sum++;
    return sum;
}


static void make_vcut(int a, int l, int w, int h, byte **pixels,
                      byte *sig, int k,
                      int s_row(byte *, int, int),
                      int s_col(byte **, int, int, int));


static void make_hcut(int a, int l, int w, int h,
                      byte **pixels, byte *f, int k,
                      int s_row(byte *, int, int),
                      int s_col(byte **, int, int, int))
{
    int cut = 0; /* how many rows are in the top part */
    int up_weight = 0;

    if (k > (int) sizeof(Fingerprint)) return;

    if (a)
    {
        int last_row_weight = 0;

        assert(w && h);

        while ((up_weight << 1) < a)
        {
            last_row_weight = s_row(pixels[cut], l, l + w - 1);
            up_weight += last_row_weight;
            cut++;
        }
        cut--;
        up_weight -= last_row_weight;
        f[k] = (byte) ((256 *
                    (cut * w + w * ((a >> 1) - up_weight) / last_row_weight))
                 / (w * h));
        if (a - (up_weight << 1) > last_row_weight)
        {
            cut++;
            up_weight += last_row_weight;
        }
    }
    else
    {
        cut = h / 2;
        f[k] = 128;
    }

    make_vcut(up_weight, l, w, cut, pixels, f, k << 1, s_row, s_col);
    make_vcut(a - up_weight, l, w, h - cut, pixels + cut, f, (k << 1) | 1, s_row, s_col);
}


static void make_vcut(int a, int l, int w, int h,
                      byte **pixels, byte *f, int k,
                      int s_row(byte *, int, int),
                      int s_col(byte **, int, int, int))
{
    int cut = 0;          /* how many columns are in the left part */
    int left_weight = 0;

    if (k > (int) sizeof(Fingerprint)) return;

    if (a)
    {
        int last_col_weight = 0;

        assert(w && h);

        while ((left_weight << 1) < a)
        {
            last_col_weight = s_col(pixels, l + cut, 0, h-1);
            left_weight += last_col_weight;
            cut++;
        }
        cut--;
        left_weight -= last_col_weight;
        f[k] = (byte) ((256 *
                    (cut * h + h * ((a >> 1) - left_weight) / last_col_weight))
                 / (w * h));
        if (a - (left_weight << 1) > last_col_weight)
        {
            cut++; left_weight += last_col_weight;
        }
    }
    else
    {
        cut = w / 2;
        f[k] = 128;
    }

    make_hcut(left_weight, l, cut, h, pixels, f, k << 1, s_row, s_col);
    make_hcut(a - left_weight, l + cut, w - cut, h, pixels, f, (k << 1) | 1, s_row, s_col);
}


static void get_fingerprint(int width, int height, byte **pixels, Fingerprint *f,
            int s_row(byte *, int, int),
            int s_col(byte **, int, int, int))
{
    int area = 0, i;
    
    for (i = 0; i < height; i++)
    {
        area += s_row(pixels[i], 0, width - 1);
    }
    assert(area >= 0);

    make_hcut(area, 0, width, height, pixels, ((unsigned char *) *f) - 1, 1, s_row, s_col);
}


void get_fingerprint_gray(unsigned char **data, int w, int h, Fingerprint *result)
{
    get_fingerprint(w, h, data, result, sum_row_gray, sum_column_gray);
}


void get_fingerprint_bw(unsigned char **data, int w, int h, Fingerprint *result)
{
    get_fingerprint(w, h, data, result, sum_row_bw, sum_column_bw);
}


long fingerprint_distance_squared(Fingerprint f1, Fingerprint f2)
{
    int i;
    long s = 0;

    for (i = 0; i < (int) sizeof(Fingerprint); i++)
    {
        long difference = f1[i] - f2[i];
        s += difference * difference;
    }

    assert(s >= 0);
    return s;
}
