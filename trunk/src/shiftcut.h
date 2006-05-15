/* Plasma OCR - an OCR engine
 *
 * shiftcut.h - compute shift-n-cut fingerprints
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


#ifndef PLASMA_OCR_SHIFTCUT_H
#define PLASMA_OCR_SHIFTCUT_H


typedef unsigned char Fingerprint[31];

void get_fingerprint_bw(unsigned char **, int w, int h, Fingerprint *result);
void get_fingerprint_gray(unsigned char **, int w, int h, Fingerprint *result);
long fingerprint_distance_squared(Fingerprint f1, Fingerprint f2);

#endif
