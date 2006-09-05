#include "conv2.h"


#define MIN_LOG_TILE_SIZE 8
#define MAX_LOG_TILE_SIZE 25    // can you imagine such a bitmap?
#define NUMBER_OF_SIZES (MAX_LOG_TILE_SIZE - MIN_LOG_TILE_SIZE + 1)



FFT2Kernel::FFT2Kernel(signed char **pixels, int w, int h)
    : width(w), height(h)
{
    cache = new (int32 **)[MAX_LOG_FFT_SIZE + 1];
    data = new int32[w][h];

    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
    {
    }
    
    XXXXXXXX copy data
}

FFT2Kernel::~FFT2Kernel()()
{
    for (int i = 0; i <= NUMBER_OF_SIZES; i++)
        free_cache(i);
    
    delete[] cache;
}

void FFT2Kernel::cache_fft(int log_size)
{
    assert(log_size <= MAX_LOG_FFT_SIZE);
    
}

void FFT2Kernel::free_cache(int log_size)
{
    assert(log_size <= MAX_LOG_FFT_SIZE);
    
}




typedef uint32 fft_int;


/* omega[i] is a root of 1 with degree 2**(i+1)
 */
static fft_int omega[MAX_LOG_TILE_SIZE];


static int     *reversal_table[NUMBER_OF_SIZES];



static void fill_with_tiles(int width, int height)
{
    
}


void conv2(signed char **kernel, int kernel_width, int kernel_height,
           signed char **image,  int image_width,  int image_height,
           int32 **result)
{
    
}
