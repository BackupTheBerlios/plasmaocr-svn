#ifndef IFFT_H
#define IFFT_H

#include <stdint.h>


enum
{
    MAX_LOG_FFT_SIZE = 25,              // that's very large!
    FFT_P = ((1<<30) + (1<<25) + 1)     // a big prime number
};

void fft(uint32_t *out, uint32_t *in, int log_n, int sign);

#ifndef NDEBUG
void sft(uint32_t *out, uint32_t *in, int log_n, int sign);
#endif


#endif
