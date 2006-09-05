#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fft.h"


enum
{
    P = FFT_P,
    G = 10,                          // primitive root modulo P
    MAX_BIT_REVERSE_CACHED = 8
};

// Multiplication modulo P
#define MUL(X,Y) ((uint64) (X) * (Y) % P)



void descale(uint32 *in, int log_n)
{
    int n = 1 << log_n;
    int coef = P - (P - 1) / n;
    for (int i = 0; i < n; i++)
        in[i] = MUL(coef, in[i]);
}



// _____________________________   global variables   ______________________________
//
// These are caches. They can be precomputed.


static int fft_initialized = 0;

/*
 * positive_omegas[i] is a root of unity with degree 2**(i+1);
 * negative_omegas[i] are 1/positive_omegas[i] mod P
 */
static uint32 positive_omegas[MAX_LOG_FFT_SIZE];
static uint32 negative_omegas[MAX_LOG_FFT_SIZE];

static int32 *reversal_table[MAX_BIT_REVERSE_CACHED];



// ______________________________   bit reversal   _________________________________

// {{{

// Reverse bits (swap 0th with (log_n-1)-th and so on).
static int32 reverse_bits(int32 a, int log_n)
{
    int32 result = 0;
    
    for (int i = 0; i < log_n; i++)
    {
        if (a & (1 << i))
            result += 1 << (log_n - i - 1);
    }
    
    return result;
}


static void fill_reversal_table(int log_n)
{
    assert(log_n <= MAX_BIT_REVERSE_CACHED);
    int n = 1 << log_n;
    
    if (reversal_table[log_n])
        return;

    int32 *table = reversal_table[log_n] = new int32[1 << log_n];
    
    for (int i = 0; i < n; i++)
        table[i] = reverse_bits(i, log_n);
}


static void bit_reverse_copy(uint32 *out, uint32 *in, int log_n)
{
    int n = 1 << log_n;

    // assert that the areas don't overlap
    assert(!(out >= in  &&  out < in + n));
    assert(!(in >= out  &&  in < out + n));
        
    if (log_n <= MAX_BIT_REVERSE_CACHED)
    {
        fill_reversal_table(log_n);
        for (int i = 0; i < n; i++)
            out[reversal_table[log_n][i]] = in[i];
    }
    else
    {
        for (int i = 0; i < n; i++)
            out[reverse_bits(i, log_n)] = in[i];
    }
}


static void bit_reverse_in_place(uint32 *A, int log_n)
{
    int n = 1 << log_n;
    if (log_n <= MAX_BIT_REVERSE_CACHED)
    {
        fill_reversal_table(log_n);
        for (int i = 0; i < n; i++)
        {
            int32 rev = reversal_table[log_n][i];
            if (i < rev)
            {
                uint32 tmp = A[i];
                A[i] = A[rev];
                A[rev] = tmp;
            }
        }
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            int32 rev = reverse_bits(i, log_n);
            if (i < rev)
            {
                uint32 tmp = A[i];
                A[i] = A[rev];
                A[rev] = tmp;
            }
        }
    }    
}

static void bit_reverse(uint32 *out, uint32 *in, int log_n)
{
    if (in == out)
        bit_reverse_in_place(in, log_n);
    else
        bit_reverse_copy(out, in, log_n);
}

// }}}


// _________________________________   omegas   ____________________________________

// {{{

static uint32 pow_mod_p(uint32 base, int exp)
{
    uint32 q;
    
    if (exp == 0)
        return 1;

    q = pow_mod_p(base, exp / 2);
    q = MUL(q, q);
        
    if (exp % 2)
        return MUL(q, base);
    else
        return q;
}


static void calc_omegas(uint32 *omegas, uint32 generator)
{
    assert(P % (1 << MAX_LOG_FFT_SIZE) == 1);
    
    uint32 w = pow_mod_p(generator, (P - 1) / (1 << MAX_LOG_FFT_SIZE));
    for (int i = MAX_LOG_FFT_SIZE; i; i--)
    {
        omegas[i - 1] = w;
        w = MUL(w, w);
    }
    assert(omegas[0] == P - 1);
}


static void fill_omegas()
{
    calc_omegas(positive_omegas, G);
    calc_omegas(negative_omegas, pow_mod_p(G, P - 2));  // G**(P-2) is just 1/G :)
    
    for (int i = 0; i < MAX_LOG_FFT_SIZE; i++)
        assert(MUL(positive_omegas[i], negative_omegas[i]) == 1);
}

// }}}


// _____________________________   init & cleanup   ________________________________

// {{{

static struct Cleaner
{
    ~Cleaner()
    {
        // this code will be run at program exit
        
        for (int i = 0; i < MAX_BIT_REVERSE_CACHED; i++)
            if (reversal_table[i])
                delete [] reversal_table[i];
        
    }
} cleaner;


static void init_fft()
{
    if (fft_initialized) return;
    fill_omegas();
    fft_initialized = 1;
}

// }}}


// _______________________________   FFT itself   __________________________________

// {{{

/* FFT without initial bit-reversal.
 * This corresponds to Cormen, Leiserson, Rivest, 32.3, "Iterative-FFT",
 * except that:
 *   1) no Bit-Reverse-Copy is called
 *   2) i == s - 1  (we have i, CLR have s)
 *   3) our twice_m is m in CLR
 */
static void raw_fft(uint32 *A, uint32 *omegas, int log_n)
{
    int n = 1 << log_n;
    for (int i = 0; i < log_n; i++)
    {
        int w = 1;
        uint32 omega_m = omegas[i];
        int m = 1 << i, twice_m = m << 1;
        
        for (int j = 0; j < m; j++)
        {
            int k;
            for (k = j; k < n; k += twice_m)
            {
                uint32 t = MUL(w, A[k + m]);
                uint32 u = A[k];
                A[k] = (u + t) % P;
                A[k + m] = (u - t + P) % P;  // + P since % for negatives works bad
            }
            w = MUL(w, omega_m);
        }
    }
}


/* This implementation is adapted from Cormen-Leiserson-Rivest, ch. 32 (Russian ed.).
 */
void fft(uint32 *out, uint32 *in, int log_n, int sign)
{
    init_fft();
    bit_reverse(out, in, log_n);
    raw_fft(out, (sign == 1 ? positive_omegas : negative_omegas), log_n);
}

// }}}


// ________________________________   testing   ____________________________________

// {{{

#if TEST > 5


/* "Slow Fourier Transform".
 * Just for testing.
 */
static void sft(uint32 *out, uint32 *in, int log_n, int sign)
{
    int n = 1 << log_n;
    uint32 *omegas = (sign == 1 ? positive_omegas : negative_omegas);
    
    memset(out, 0, n * sizeof(uint32));
    uint32 w = omegas[log_n - 1];
    
    for (int i = 0; i < n; i++)
    {
        int j;

        for (j = 0; j < n; j++)
        {
            out[i] = (out[i] + (uint64) in[j] * pow_mod_p(w, i * j)) % P;
        }
    }
}

static void assert_eq_N(uint32 *a1, uint32 *a2, int n)
{
    assert(!memcmp(a1, a2, n * sizeof(uint32)));
}



/* Also test that they don't change input.
 */
static void assert_fft_equals_sft(uint32 *in, int log_n, int sign)
{
    int n = 1 << log_n;
    uint32 in_copy[n], out_fft[n], out_sft[n];
    memcpy(in_copy, in, n * sizeof(uint32));
    fft(out_fft, in, log_n, sign);
    assert_eq_N(in, in_copy, n);
    sft(out_sft, in, log_n, sign);
    assert_eq_N(in, in_copy, n);
    
    for (int i = 0; i < n; i++)
        assert(out_fft[i] == out_sft[i]);
}


static void assert_fft_ifft_is_id(uint32 *in, int log_n, int sign)
{
    int n = 1 << log_n;
    uint32 out_fft[n], out_ifft[n];
    fft(out_fft, in, log_n, sign);
    fft(out_ifft, out_fft, log_n, -sign);
    descale(out_ifft, log_n);
    for (int i = 0; i < n; i++)
        assert(out_ifft[i] == in[i]);
}


static void assert_in_place_works(uint32 *in, int log_n, int sign)
{
    int n = 1 << log_n;
    uint32 in_copy[n], out_fft[n];
    memcpy(in_copy, in, n * sizeof(uint32));
    fft(out_fft, in, log_n, sign);
    fft(in_copy, in_copy, log_n, sign);
    for (int i = 0; i < n; i++)
        assert(out_fft[i] == in_copy[i]);    
}


static void test_sanity()
{
    uint64 b;
    /* Check that uint32 can hold P-1. */
    uint32 p = P - 1;
    assert(p == P - 1);

    /* Check that uint64 can hold P-1 squared. */
    b = (uint64) p * p;
    assert(b == (uint64) p * p);
    assert(sizeof(uint32) == 4);
    assert(sizeof(uint64) == 8);
}


static void test_fft()
{
    uint32 in[32];

    for (int log_n = 0; log_n < 5; log_n++)
    {
        int n = 1 << log_n;
        for (int k = 0; k < n; k++)
        {
            memset(in, 0, sizeof(in));
            in[k] = 1;
            assert_fft_equals_sft(in, log_n,  1);
            assert_fft_equals_sft(in, log_n, -1);
            assert_fft_ifft_is_id(in, log_n,  1);
            assert_fft_ifft_is_id(in, log_n, -1);
            assert_in_place_works(in, log_n,  1);
            assert_in_place_works(in, log_n, -1);
        }
    }
}


static struct Tester
{
    Tester()
    {
        // this code will be run at startup

        puts("testing fft");
        test_sanity();
        test_fft();
    }
} tester;


#endif

// }}}
