/* Plasma OCR - an OCR engine
 *
 * inftycdb.h - parsing InftyCDB database
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


#ifndef INFTYCDB_H
#define INFTYCDB_H


typedef enum
{
    LINK_TOP,
    LINK_HORIZONTAL,
    LINK_UPPER,
    LINK_LOWER,
    LINK_LEFT_SUBSCRIPT,
    LINK_LEFT_SUPERSCRIPT,
    LINK_RIGHT_SUBSCRIPT,
    LINK_RIGHT_SUPERSCRIPT
} InftyLink;


typedef struct
{
    int char_ID;
    int article_ID;
    int page;
    char type[20];
    char code[5];
    char entity[30];
    int is_math;
    int on_main_line; // subscripts and supersctipts are not on_main_line
    int is_italic;
    int is_bold;
    int is_touched;
    int is_separated;
    int quality_unknown;
    int is_doubleprint;
    int width;
    int height;
    int parent;
    InftyLink link;
    char path_to_image[12];
    int left, top, right, bottom;
    int word_ID;
    char MathML[5000];
    char TeX[5000];
    char IML[5000];
    int word_left, word_top, word_right, word_bottom;
    int is_hyphenated;
} InftyRecord;


extern InftyRecord infty_record;


void infty_open(const char *path_to_bzipped2_csv_database);
int infty_read(); /* zero - EOF */
void infty_close();


#endif
