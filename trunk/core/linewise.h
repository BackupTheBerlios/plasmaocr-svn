/* Plasma OCR - an OCR engine
 *
 * linewise.h - reading text files line by line
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



#ifndef PLASMA_OCR_LINEWISE_H
#define PLASMA_OCR_LINEWISE_H


#include <stdio.h>


typedef struct LinewiseReaderStruct *LinewiseReader;

LinewiseReader linewise_reader_create(FILE *input);

/* Destroy the LinewiseReader and close the file if necessary. */
void linewise_reader_destroy(LinewiseReader);

/* Read a line (including newline character), allocating memory as necessary.
 * Returns NULL and closes the file in case of EOF.
 * Important: with each call, previously allocated lines are erased.
 */
const char *linewise_read(LinewiseReader);

/* Same as linewise_read, but strips newline character. */
const char *linewise_read_and_chomp(LinewiseReader r);


#endif /* LINEWISE_H */
