/* Plasma OCR - an OCR engine
 *
 * fft.c - Fast Fourier Transform (in finite field)
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


/* Our goal is fast convolution.
 * Kernels should not be larger then a typical letter size; 128 x 128 is big enough.
 * Moreover, we don't need large values in kernels, and our bitmap contains -1 and 1.
 *  (Note: it is convenient here to assume -1 == white, 1 == black, 0 == don't care).
 * Suppose that our kernel contains only -1, 1 and 0. Obviously, the convolution result
 * will be in interval -128*128 .. 128*128.
 * That means that we don't really need real or complex values, a finite field will do.
 */


#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>


/* The length of data block. */
#define LOG_N 8
#define N (1 << LOG_N)

/* A prime number such that P == 1 (mod N).
 * If our template contains T nonzero pixels, than we want  P  >  2 * T.
 * (That is for bitmap convolutions. For graymaps, it'll be much more tricky.)
 */
#define P 64513 // == 252 * 256 + 1
#define G 5     // primitive root of P

/* other alternatives (k such that 256*k+1) is prime:
 *
 * 1 3 13 30 31 37 42 46 48 52 55 57 60 70 72 76 87 90 91 100 102 105 121 123
 * 126 141 142 147 148 156 157 160 163 171 178 181 193 195 196 202 223 225 228
 * 232 235 240
 */



/* An unsigned integer type that can to hold numbers up to P-1.
 * Moreover, we're going to multiply and then % P, and it should work too.
 */
typedef unsigned short fft_int;

/* Should be at least twice as long as fft_int.
 * (We'll multiply two fft_ints into big_int)
 */
typedef unsigned long big_int;


/* omega[i] is a root of unity with degree 2**(i+1)
 */
static fft_int omega[LOG_N];
static int     reversal_table[N];


static fft_int pow_mod_p(fft_int base, int exp)
{
    fft_int q;
    
    if (exp == 0)
        return 1;

    q = pow_mod_p(base, exp / 2);
    q = (big_int) q * q % P;
        
    if (exp % 2)
        return (big_int) q * base % P;
    else
        return q;
}


static void fill_omega()
{
    int i;
    assert((P-1) % N == 0);
    fft_int w = pow_mod_p(G, (P - 1) / N);
    for (i = LOG_N; i; i--)
    {
        omega[i - 1] = w;
        w = (big_int) w * w % P;
    }
    assert(omega[0] == P - 1);
}


int reverse_bits_in_byte(int b)
{
    int result = 0;
    int i;
    for (i = 0; i < LOG_N; i++)
    {
        if (b & (1 << i))
            result += 1 << (LOG_N - i - 1);
    }
    return result;
}


void in_place_bit_reverse(fft_int *A)
{
    int i;
    for (i = 0; i < N; i++)
    {
        int dest = reversal_table[i];
        if (dest < i)
            A[dest] = A[i];
    }
}


void bit_reverse(fft_int *out, fft_int *in)
{
    int i;
    for (i = 0; i < N; i++)
        out[reversal_table[i]] = in[i];
}



/* FFT without initial reversal */
void raw_fft(fft_int *out, fft_int *in)
{
    int i;
    for (i = 0; i < LOG_N; i++)
    {
        int w = 1;
        fft_int omega_m = omega[i];
        int m = 1 << i, twice_m = m << 1;
        int j;
        
        for (j = 0; j < m; j++)
        {
            int k;
            for (k = j; k < N; k += twice_m)
            {
                fft_int t = (big_int) w * out[k + m] % P;
                fft_int u = out[k];
                out[k] = (u + t) % P;
                out[k + m] = (u - t + P) % P;  // + P since % for negatives works bad
            }
            w = (big_int) w * omega_m % P;
        }
    }
}


/* This implementation is adapted from Cormen-Leiserson-Rivest, ch. 32 (Russian ed.).
 */
void fft(fft_int *out, fft_int *in)
{
    bit_reverse(out, in);
    raw_fft(out, in);
}



/*void fft2(fft_int **tile)
{
    int i;
    for (i = 0; i < N; i++)
        in_place_fft(tile[i]);
    
    transpose(tile);
    
    for (i = 0; i < N; i++)
        in_place_fft(tile[i]);
    
    transpose(tile);
}*/

void init_fft()
{
    int i;
    for (i = 0; i < N; i++)
    {
        reversal_table[i] = reverse_bits_in_byte(i);
    }
    fill_omega();
}


/* ___________________________________________________________________________________ */

#ifdef TESTING


/* "Slow Fourier Transform".
 * Just for testing.
 */
void sft(fft_int *out, fft_int *in)
{
    int i;
    
    memset(out, 0, N * sizeof(fft_int));
    fft_int w = omega[LOG_N - 1];
    
    for (i = 0; i < N; i++)
    {
        int j;

        for (j = 0; j < N; j++)
        {
            out[i] = (out[i] + (big_int) in[j] * pow_mod_p(w, i * j)) % P;
        }
    }
}

static void assert_eq_N(fft_int *a1, fft_int *a2)
{
    assert(!memcmp(a1, a2, N * sizeof(fft_int)));
}

/*static void print(fft_int *A)
{
    int i;
    for (i = 0; i < N; i++)
    {
        printf("%d", A[i]);
    }
    printf("\n");
}*/



/* Also test that they don't change input.
 */
void assert_fft_equals_sft(fft_int *in)
{
    int i;
    fft_int in_copy[N], out_fft[N], out_sft[N];
    memcpy(in_copy, in, N * sizeof(fft_int));
    fft(out_fft, in);
    assert_eq_N(in, in_copy);
    sft(out_sft, in);
    assert_eq_N(in, in_copy);
    
    for (i = 0; i < N; i++)
    {
        assert(out_fft[i] == out_sft[i]);
    }
}


static void test_sanity()
{
    big_int b;
    /* Check that fft_int can hold P-1. */
    fft_int p = P - 1;
    assert(p == P - 1);

    /* Check that big_int can hold P-1 squared. */
    b = p * p;
    assert(b == p * p);
}


// Unfortunately, we can't test many inputs because SFT is really slow
// (esp. under Valgrind).
static void test_fft_equals_sft()
{
    int i;
    fft_int in[N];
    for (i = 0; i < N; i++)
        in[i] = (2*N*N + 5*N + 1) % P;
    
    assert_fft_equals_sft(in);
}

static TestFunction tests[] = {
    test_fft_equals_sft,
    test_sanity,
    NULL
};

TestSuite fft_suite = {"fft", init_fft, NULL, tests};

#endif
