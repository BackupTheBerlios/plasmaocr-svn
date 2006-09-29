#ifndef IFFT_H
#define IFFT_H

#include <stdint.h>


enum
{
    MAX_LOG_FFT_SIZE = 27,              // that's very large!
    FFT_P_PLUS = 31,
    FFT_P_MINUS = 27,
    FFT_P = ((1U<<FFT_P_PLUS) - (1<<FFT_P_MINUS) + 1),     // a big prime number
    FFT_P_INV_32 = 2281701377U
};

void fft(uint32_t *out, uint32_t *in, int log_n, int sign);

#ifndef NDEBUG
void sft(uint32_t *out, uint32_t *in, int log_n, int sign);
void fft_test();
uint32_t invert_fft_prime();

#endif


#endif
