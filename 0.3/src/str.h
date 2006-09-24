#ifndef PLASMA_OCR_STRING_H
#define PLASMA_OCR_STRING_H

#include "common.h"
#include <assert.h>

PLASMA_BEGIN;

class String
{
    class Buffer
    {
        int link_counter;
        char *str;

        public:

            inline void link()
            {
                link_counter++;
            }

            inline void unlink()
            {
                link_counter--;
                if (!link_counter)
                    delete this;
            }
        
            inline operator const char *()
            {
                return str;
            }
            
            Buffer(const char *s);
            ~Buffer();
    };

    Buffer *buffer;
    
public:
    
    inline String(const String &s) : buffer(s.buffer)
    {
        buffer->link();
    }
    
    inline const String &operator =(const String &s)
    {
        s.buffer->link();
        buffer->unlink();
        buffer = s.buffer;
        return *this;
    }
    
    inline operator const char *()
    {
        return *buffer;
    }
    
    inline String(const char *s) : buffer(new Buffer(s)) {}

    inline ~String()
    {
        buffer->unlink();
    }
};

PLASMA_END;

#endif
