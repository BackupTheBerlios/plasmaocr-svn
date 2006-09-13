#ifndef PLASMA_OCR_CORE_H
#define PLASMA_OCR_CORE_H


typedef enum
{
    CC_BLACK,   // only shallow match
    CC_RED,     // no shallow match
    CC_GREEN,   // only deep match
    CC_YELLOW,  // best shallow, no deep match
    CC_MAGENTA, // uncertainly recognized by two methods
    CC_BLUE     // more than one deep matches
} ColorCode;

typedef struct
{
    ColorCode color;
    const char *text;
} Opinion;

Opinion *core_recognize_letter(Library, int no_blacks, unsigned char **pixels, int width, int height);
void core_destroy_opinion(Opinion *);

#endif
