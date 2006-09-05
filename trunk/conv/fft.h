#ifndef FFT_H
#define FFT_H


// XXX! better porting
typedef int int32;
typedef unsigned uint32;
typedef unsigned long long uint64;

enum {
    MAX_LOG_FFT_SIZE = 25,              // that's very large!
    FFT_P = ((1<<30) + (1<<25) + 1)     // a big prime number
};

/* FFT_P should be 1 modulo 2**MAX_LOG_FFT_SIZE. */


/* Compute FFT of `in' and place it into `out'.
 * The FFT is computed over a finite field (modulo FFT_P).
 * The sign might be 1 or -1; the inverse transform is non-scaled.
 *
 * To perform in-place FFT, set in == out.
 * Note: in and out may not overlap except for the case in == out.
 */
void fft(uint32 *out, uint32 *in, int log_n, int sign);


/* After consecutive FFTs in opposite directions,
 *  the input becomes scaled by n.
 * This function divides back the given vector by n.
 */
void descale(uint32 *, int log_n);


#endif
