/* Plasma OCR - an OCR engine
 *
 * inftycdb.c - parsing InftyCDB database
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


#include <bzlib.h>  // -lbz2
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "inftycdb.h"


// alas, global variables
// code in a column rocks
InftyRecord infty_record;
static BZFILE *bz = NULL;
static char buffer[8192];
static int buffer_length;
static int records_count;
// like a poem in code :)
// too bad the rest of it
// would be harder to fit


static struct
{
    InftyQuality quality;
    char *string;
} quality_strings[] = {
    {QUALITY_NORMAL,                "normal"},
    {QUALITY_TOUCHED,               "touched"},
    {QUALITY_SEPARATED,             "separate"},
    {QUALITY_TOUCHED_AND_SEPARATED, "touch_and_sep"},
    {QUALITY_UNKNOWN,               ""},
    {QUALITY_DOUBLEPRINT,           "doubleprint"}
};


static struct
{
    InftyLink link;
    char *string;
} link_strings[] = {
    {LINK_TOP, "TOP"},
    {LINK_HORIZONTAL, "HORIZONTAL"},
    {LINK_UPPER, "UPPER"},
    {LINK_LOWER, "UNDER"},
    {LINK_LEFT_SUBSCRIPT, "LSUB"},
    {LINK_LEFT_SUPERSCRIPT, "LSUP"},
    {LINK_RIGHT_SUBSCRIPT, "RSUB"},
    {LINK_RIGHT_SUPERSCRIPT, "RSUP"}
};


void infty_open(const char *path)
{
    infty_close();
    bz = BZ2_bzopen(path, "rb");
    if (!bz)
    {
        fprintf(stderr, "infty_open> cannot open %s\n", path);
        exit(1);
    }

    buffer_length = records_count = 0;
}


static int parse_int(char *s)
{
    char *endptr;
    long result = strtol(s, &endptr, 10);
    if (!s || *endptr)
    {
        fprintf(stderr, "parse_int> problem: %s\n", s);
        exit(1);
    }
    return (int) result;
}


static int parse_boolean(char *s)
{
    int result = parse_int(s);
    if (result != 0  &&  result != 1)
    {
        fprintf(stderr, "parse_boolean> %s is neither 0 nor 1\n", s);
        exit(1);
    }
    return result;
}


static void parse_string(char *s, char *dest, int length)
{
    int written = 0;
    if (*s != '"')
    {
        fprintf(stderr, "parse_string> string %s was expected to start with \"\n", s);
        exit(1);
    }

    while (1)
    {
        s++;
        if (!*s)
        {
            fprintf(stderr, "parse_string> string was not closed\n");
            exit(1);
        }

        if (*s == '"')
        {
            s++;
            if (*s == '"')
                dest[written++] = '"'; // escape sequence: "" -> "
            else if (!*s)
            {
                dest[written] = '\0';  // that was final "
                return;
            }
            else
            {
                fprintf(stderr, "parse_string> unexpected \" finished by %s\n", s);
                exit(1);
            }
        }
        else
            dest[written++] = *s; // normal char

        // raising alarm one char earlier, we have yet to put a \0 there
        if (written == length)
        {
            fprintf(stderr, "parse_string> string too long (buffer is only %d), tail is %s\n", length, s);
            exit(1);
        }
    }
}


// well, kinda silly that this is a partial reimplementation of parse_line
static char *skip_to_next_unmatched_quote(char *s)
{
    while (1)
    {
        s++;
        if (!*s)
            return s;

        if (*s == '"')
        {
            s++;
            if (*s != '"')
                return s;
        }
    }
}


static char *skip_to_comma_or_eol(char *s)
{
    while (*s && *s != ',') s++;
    return s;
}


static char *skip_field(char *s)
{
    if (*s == '"')
        return skip_to_next_unmatched_quote(s);
    else
        return skip_to_comma_or_eol(s);
}


static int split_csv(char *line, char **fields, int fields_max)
{
    int fields_written = 0;

    while (1)
    {
        char *end_line = skip_field(line);
        if (fields_written == fields_max)
        {
            fprintf(stderr, "split_csv> unexpectedly many fields in %s\n", line);
            exit(1);
        }

        fields[fields_written++] = line;
        if (!*end_line)
            return fields_written;

        *end_line = '\0';
        line = end_line + 1;
    }
}


static void parse_line()
{
    char *fields[29];
    char region[7];
    char quality[14];
    char link[12];
    const int total_qualities = sizeof(quality_strings) / sizeof(*quality_strings);
    const int total_links     = sizeof(   link_strings) / sizeof(*   link_strings);
    int i;

    int expected_fields_count = sizeof(fields) / sizeof(char *);
    int fields_read = split_csv(buffer, fields, expected_fields_count);

    if (fields_read != expected_fields_count)
    {
        fprintf(stderr, "unexpected number of fields on record %d: %d\n", records_count, fields_read);
        exit(1);
    }

    infty_record.char_ID       = parse_int(fields[0]);
    infty_record.article_ID    = parse_int(fields[1]);
    infty_record.page          = parse_int(fields[2]);
    infty_record.on_main_line  = parse_boolean(fields[7]);
    infty_record.is_italic     = parse_boolean(fields[8]);
    infty_record.is_bold       = parse_boolean(fields[9]);
    infty_record.width         = parse_int(fields[11]);
    infty_record.height        = parse_int(fields[12]);
    infty_record.parent        = parse_int(fields[13]);
    infty_record.left          = parse_int(fields[16]);
    infty_record.top           = parse_int(fields[17]);
    infty_record.right         = parse_int(fields[18]);
    infty_record.bottom        = parse_int(fields[19]);
    infty_record.word_ID       = parse_int(fields[20]);
    infty_record.word_left     = parse_int(fields[24]);
    infty_record.word_top      = parse_int(fields[25]);
    infty_record.word_right    = parse_int(fields[26]);
    infty_record.word_bottom   = parse_int(fields[27]);
    infty_record.is_hyphenated = parse_int(fields[28]);

    #define PARSE_STRING(N, NAME) parse_string(fields[N], infty_record.NAME, sizeof(infty_record.NAME))
    PARSE_STRING(3, type);
    PARSE_STRING(4, code);
    PARSE_STRING(5, entity);
    PARSE_STRING(15, path_to_image);
    PARSE_STRING(21, MathML);
    PARSE_STRING(22, TeX);
    PARSE_STRING(23, IML);

    parse_string(fields[6], region, sizeof(region));
    if (!strcmp(region, "text"))
        infty_record.is_math = 0;
    else if (!strcmp(region, "math"))
        infty_record.is_math = 1;
    else
    {
        fprintf(stderr, "in record %d, region field is neither text nor math\n", infty_record.char_ID);
        exit(1);
    }

    parse_string(fields[10], quality, sizeof(quality));
    for (i = 0; i < total_qualities; i++)
    {
        if (!strcmp(quality, quality_strings[i].string))
        {
            infty_record.quality = quality_strings[i].quality;
            break;
        }
    }

    if (i == total_qualities)
    {
        fprintf(stderr, "in record %d, quality field is unknown: %s\n", records_count, quality);
        exit(1);
    }

    parse_string(fields[14], link, sizeof(link));

    for (i = 0; i < total_links; i++)
    {
        if (!strcmp(link, link_strings[i].string))
        {
            infty_record.link = link_strings[i].link;
            break;
        }
    }

    if (i == total_links)
    {
        fprintf(stderr, "parse_line> unknown link type: %s\n", link);
        exit(1);
    }

    if (infty_record.right - infty_record.left + 1  !=  infty_record.width
     || infty_record.bottom - infty_record.top + 1  !=  infty_record.height)
    {
        fprintf(stderr, "parse_line> inconsistent bounding box\n");
        exit(1);
    }

    /* This test doesn't hold.
     * The first line on which char_ID and line number don't equal is 1749.

    if (infty_record.char_ID != records_count)
    {
        fprintf(stderr, "parse_line> char ID %d at line %d\n", infty_record.char_ID, records_count);
        exit(1);
    }
    */
}


static int find_line_break()
{
    int i;

    for (i = 0; buffer[i] && i < buffer_length; i++)
        if (buffer[i] == '\n' || (buffer[i] == '\r' && i + 1 < buffer_length))
            return i;

    return -1;
}


int infty_read()
{
    int line_break = find_line_break();

    if (line_break == -1)
    {
        int bytes_read = BZ2_bzread(bz, buffer + buffer_length,
                                    sizeof(buffer) - buffer_length);
        if (!bytes_read)
        {
            infty_close();
            return 0;
        }

        buffer_length += bytes_read;

        line_break = find_line_break();
        if (line_break == -1)
        {
            fprintf(stderr, "infty_read> line break not found!"
                            "bytes_read = %d, buffer_length = %d\n",
                            bytes_read, buffer_length);
            exit(1);
        }
    }

    buffer[line_break] = '\0';
    parse_line();
    records_count++;

    if (line_break + 1 < buffer_length  &&  buffer[line_break + 1] == '\n')
        line_break++;
    buffer_length -= line_break + 1;
    memmove(buffer, buffer + line_break + 1, buffer_length);
    return 1;
}


void infty_close()
{
    if (bz)
    {
        BZ2_bzclose(bz);
        bz = NULL;
    }
}


static void dump_string(FILE *f, const char *s)
{
    fputc('"', f);
    while (*s)
    {
        if (*s == '"')
            fputc('"', f);
        fputc(*s, f);
        s++;
    }
    fputc('"', f);
}


void infty_regenerate(const char *src_path, const char *dest_path)
{
    FILE *f = fopen(dest_path, "w");
    infty_open(src_path);
    while (infty_read())
    {
        int i;
        const int total_qualities = sizeof(quality_strings) / sizeof(*quality_strings);
        const int total_links     = sizeof(   link_strings) / sizeof(*   link_strings);

        fprintf(f, "%d,%d,%d,", infty_record.char_ID,
                                infty_record.article_ID,
                                infty_record.page);

        dump_string(f, infty_record.type);   fputc(',', f);
        dump_string(f, infty_record.code);   fputc(',', f);
        dump_string(f, infty_record.entity); fputc(',', f);

        if (infty_record.is_math)
            dump_string(f, "math");
        else
            dump_string(f, "text");
        fputc(',', f);

        fprintf(f, "%d,%d,%d,", infty_record.on_main_line,
                                infty_record.is_italic,
                                infty_record.is_bold);
        for (i = 0; i < total_qualities; i++)
        {
            if (infty_record.quality == quality_strings[i].quality)
            {
                dump_string(f, quality_strings[i].string);
                break;
            }
        }
        assert(i < total_qualities);

        fprintf(f, ",%d,%d,%d,", infty_record.width,
                                 infty_record.height,
                                 infty_record.parent);

        for (i = 0; i < total_links; i++)
        {
            if (infty_record.link == link_strings[i].link)
            {
                dump_string(f, link_strings[i].string);
                break;
            }
        }
        assert(i < total_links);

        fputc(',', f);
        dump_string(f, infty_record.path_to_image);
        fprintf(f, ",%d,%d,%d,%d,%d,", infty_record.left,
                                       infty_record.top,
                                       infty_record.right,
                                       infty_record.bottom,
                                       infty_record.word_ID);

        dump_string(f, infty_record.MathML);
        fputc(',', f);
        dump_string(f, infty_record.TeX);
        fputc(',', f);
        dump_string(f, infty_record.IML);

        fprintf(f, ",%d,%d,%d,%d,%d\r\n", infty_record.word_left,
                                          infty_record.word_top,
                                          infty_record.word_right,
                                          infty_record.word_bottom,
                                          infty_record.is_hyphenated);
    }
    fclose(f);
}
