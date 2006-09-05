#ifndef CONV2_H
#define CONV2_H


enum { MAX_LOG_KERNEL_SIZE = 7 };


/* A kernel is a graymap (with colors from -128..127).
 * The size of the kernel should be no more than 128x128.
 *
 * (This can be adjusted by setting MAX_LOG_KERNEL_SIZE,
 *  but note that the answers must fit in some integer).
 *
 * FLIPPING: the kernel is flipped before convolution!
 * So, if you're searching for a letter on a page,
 * the letter image must NOT be flipped.
 *
 * This class exists to cache the FFT results.
 */
class FFT2Kernel
{
    public:
        const int width, height;
        
        FFT2Kernel(signed char **pixels, int width, int height);
        virtual ~FFT2Kernel();

    private:
        int32 **data;
        int32 ***cache;

        void cache_fft(int log_size);
        void free_cache(int log_size);
    
    friend class FFT2Conv;
};


/* FFT2Image is a graymap (-128..127).
 * Thiss class exists to cache the tiling and FFT results.
 */
class FFT2Image
{
    public:
        const int width, height;
        
        FFT2Image(signed char **pixels, int width, int height);
        virtual ~FFT2Image();
    private:
        class FFT2Tile;
        FFT2Tile *tiles;
        
    friend class FFT2Conv;
};


/* This is the convolution result.
 * The computation is done by the constructor.
 * The result's width is equal to (image's width - kernel's width + 1);
 * same for the height.
 */
class FFT2Conv
{
    public:
        const int width, height;
        int32 * const * const result;
        
        FFT2Conv(FFT2Kernel &, FFT2Image &);
        virtual ~FFT2Conv();
};


#endif
