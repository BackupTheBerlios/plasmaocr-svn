#ifndef PLASMA_OCR_FILE_H
#define PLASMA_OCR_FILE_H

#include "common.h"
#include <stdio.h>
#include <assert.h>


PLASMA_BEGIN;

/** \brief A file stream.
 *
 * A File object is basically the same as stdio's FILE.
 * It supports many operations from stdio.
 */
class File
{
    FILE *stream;
    bool ownership;
    
    File &operator =(const File &f)
    {
        assert(!"Copying of File objects isn't permitted!");
    }

public:
    /** \brief Opens a file.
     *
     * If the file name is "-", gives stdin or stdout (not compatible with r/w mode).
     * 
     * Besides fopen() modes, this constructor offers some more:
     *   - "+" - open a file for reading and writing, creating if necessary,
     *           but not overwriting if it exists.
     *   - "+b" or "b+" - same, but in binary mode 
     *   - "tmp" - open a temporary file in binary mode (path ignored)
     */
    File(const char *path, const char *mode);
    inline File(FILE *file, bool own = false): stream(file), ownership(own) {}
    ~File();

    inline int puts(const char *s)   { return fputs(s, stream); }
    inline long tell()               { return ftell(stream); }
    inline bool eof()                { return (bool) feof(stream); }
    inline bool error()              { return (bool) ferror(stream); }
    inline int flush()               { return fflush(stream); }
    inline int seek(long offset, int whence) { return fseek(stream, offset, whence); }
    inline int seek_cur(long offset) { return seek(offset, SEEK_CUR); }
    inline int seek_end(long offset) { return seek(offset, SEEK_END); }
    inline int seek_set(long offset) { return seek(offset, SEEK_SET); }
    
    inline void use_as_stdin()  { stdin  = stream; }
    inline void use_as_stdout() { stdout = stream; }
    inline void use_as_stderr() { stderr = stream; }
    
    inline size_t write(void *ptr, size_t size, size_t nmemb)
    {
        return fwrite(ptr, size, nmemb, stream);
    }

    inline size_t read(void *ptr, size_t size, size_t nmemb)
    {
        return fread(ptr, size, nmemb, stream);
    }
    
    int printf(const char *format, ...);
    int scanf(const char *format, ...);

    inline void write_uint8(uint8 i)    { fputc(i, stream); }
    inline void write_uint16(uint16 i)  { write_uint8(i >> 8); write_uint8(i); }
    inline void write_uint32(uint32 i)  { write_uint16(i >> 16); write_uint16(i); }
    inline void write_uint64(uint64 i)  { write_uint32(i >> 32); write_uint32(i); }
    
    inline void write_int8(int8 i)      { write_uint8(i); }
    inline void write_int16(int16 i)    { write_uint16(i); }
    inline void write_int32(int32 i)    { write_uint32(i); }
    inline void write_int64(int64 i)    { write_uint64(i); }

    inline uint8 read_uint8()  { return fgetc(stream); }
    inline uint16 read_uint16()
    {
        uint16 x = read_uint8();
        return (x << 8) | read_uint8(); 
    }

    inline uint32 read_uint32()
    {
        uint32 x = read_uint16();
        return (x << 16) | read_uint16(); 
    }
    
    inline uint64 read_uint64()
    {
        uint64 x = read_uint32();
        return (x << 32) | read_uint32(); 
    }
    
    inline int8  read_int8()     { return read_uint8(); }
    inline int16 read_int16()    { return read_uint16(); }
    inline int32 read_int32()    { return read_uint32(); }
    inline int64 read_int64()    { return read_uint64(); }
};

/** \todo comment
 */
class FilePair
{
public:
    File *read();
    File *write();
};

PLASMA_END;

#endif
