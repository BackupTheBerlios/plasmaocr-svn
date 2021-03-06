#include "ifft.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/** \file ifft.cpp
 * \brief Integer FFT modulo a prime.
 */


// utility routines (thue_morse, reverse_bits) {{{

/** Thue-Morse sequence: 0110100110010110...
 * Argument: index, starting from 0
 * Returns: 0 or 1
 */
static int thue_morse(unsigned a)
{
    // I've always wanted to write that somewhere:
    //
    // Guessing how this implementation works
    // is left as an excercise to the reader!
    
    for (unsigned i = 1; i < sizeof(a) * 8; i <<= 1)
        a ^= a >> i;
    return a & 1;
}


// Reverse bits (swap 0th with (log_n-1)-th and so on).
static unsigned reverse_bits(unsigned a, int log_n)
{
    unsigned result = 0;
    
    for (int i = 0; i < log_n; i++)
    {
        if (a & (1 << i))
            result += 1 << (log_n - i - 1);
    }
    
    return result;
}

// utility routines (thue_morse, reverse_bits) }}}




/**
 * DNum - unsigned integer type twice as large as Num
 * primitive_root - a primitive_root modulo P
 * log_chunk_size - log_2 of the size of a chunk (most likely, should be cache size) 
 */
template <int P, int S, class Num, class DNum, Num primitive_root>
class Fourier
{
    const int threshold;
    const int log_chunk_size;
    const int half_chunk_size;
    const int chunk_size;
    Num *buffer;
    Num positive_omegas[S], negative_omegas[S];
 
    enum
    {
        optimization_offset = 16,   // this helps very little
    };

    // representations and arithmetic modulo P {{{
    

    /*static inline Num convert_2_to_1(Num a)
    {
        // Note:
        //      a >> S  ==  a/2**S
        //      a & P   ==  a % 2**S.
        //
        // a >> S might be 1, but since ~0 is forbidden for [2],
        // an overflow cannot possibly occur.
        return (a >> S) + (a & P);
    }

    #ifdef DENSE
        static inline Num signed_convert_2_to_1(Num a)
        {
            return (a & P) - ((a >> S) & ((P << 1) + 1));
        }
    #endif*/



    static inline Num add(Num a, Num b)
    {
        Num s = a + b;
        return (s >= P  ?  s - P  :  s);
    }

    static inline Num sub(Num a, Num b)
    {
        Num d = P + a - b;
        return (d >= P  ?  d - P  :  d);
    } 



    /* Montgomery reduction.
     * I've read about this trick in an article by Tommy F\"arnqvist.
     * 
     * x < 2^31 * P
     * result < P, equal to x / 2^31 (mod P)
     */
    static inline Num montgomery31(DNum x)
    {
        // x == 2^{31} * x1 + x0
        Num x0 = (Num) x & 0x7FFFFFFF;
        Num x1 = (Num) (x >> 31);

        // 2^{FFT_P_MINUS} + 1 is inverse to P modulo 2^{31}:
        // (1 - 2^{FFT_P_MINUS}) (1 + 2^{FFT_P_MINUS)) = 1 - (...)^2 = 1
        //
        // Here we set `t' to be x0 * P^{-1} modulo 2^{31}.
        // So we'll have  t * P == x (mod 2^{31}).
        //Num t = ((x0 << FFT_P_MINUS) + x0) & 0x7FFFFFFF;
        Num t = (x0 * ((1 << FFT_P_MINUS) + 1)) & 0x7FFFFFFF;

        // u = t * P / 2^{31} 
        // Note that t * P % 2^{31} == x0 by construction of t.
        // So we can put it this way: (t * P - x0) / 2^{31}, where `/' has no remainder.
        Num u = ((DNum) t * P) >> FFT_P_PLUS;
        
        if (x1 >= u)
            return x1 - u;
        else
            return P + x1 - u;
    }
    

    // multiply x by 2^31 (mod P)
    static inline Num montgomery31_into(Num x)
    {
        return ((DNum) x << 31) % P;
    }

    // divide x by 2^31 (mod P)
    static inline Num montgomery31_from(Num x)
    {
        return montgomery31(x);
    }    


    static inline Num mul(Num a, Num b)
    {
        return montgomery31((DNum) a * b);
    }
    
    // representations and arithmetic modulo P }}}


 
    
    /** This is COBRA ("Cache-Optimal Bit-Reverse Algorithm")
     * from the paper "Towards an Optimal Bit-Reversal Permutation Program"
     * by Larry Carter and Kang Su Gatlin.
     */
    void cobra(Num *out, Num *in, int log_n, int q)
    {
        int a, c;
        int cache_cap = 1 << q;
        int b_len = log_n - q - q;
        int s = log_n - q;
        int b_cap = 1 << b_len;
        for (int b = 0; b < b_cap; b++)
        {
            int b_rev = reverse_bits(b, b_len);
            for (a = 0; a < cache_cap; a++)
            {
                int a_rev = reverse_bits(a, q);
                for (c = 0; c < cache_cap; c++)
                    buffer[(a_rev << q) | c] = in[(a << s) | (b << q) | c];
            }
            
            for (c = 0; c < cache_cap; c++)
            {
                int c_rev = reverse_bits(c, q);
                for (a = 0; a < cache_cap; a++)
                    out[(c_rev << s) | (b_rev << q) | a] = buffer[(a << q) | c];
            }
        }
    }
    

    void cobra(Num *out, Num *in, int log_n)
    {
        int q1 = log_n / 2;
        int q2 = log_chunk_size / 2;
        int q = (q1 > q2  ?  q2  :  q1);
        cobra(out, in, log_n, q);
    }
    
    
    static void fft_level_0(Num *A, Num *omegas, int n)
    {
        for (int k = 0; k < n; k += 2)
        {
            Num t = A[k + 1];
            Num u = A[k];
            A[k] = add(u, t);
            A[k + 1] = sub(u, t);
        }
    }


    
    /* FFT without bit-reverse.
     * This corresponds to Cormen, Leiserson, Rivest, 32.3, "Iterative-FFT",
     * except that:
     *   1) no Bit-Reverse-Copy is called
     *   2) i == s - 1  (we have i, CLR have s)
     *   3) our twice_m is m in CLR
     *   4) first level is separate
     *   5) inner loop is repeated to exclude multiplication by 1
     */
    static void fft_raw(Num *A, Num *omegas, int n, int levels_to_go)
    {
        if (levels_to_go == 0) return;
        fft_level_0(A, omegas, n);
        
        for (int i = 1; i < levels_to_go; i++)
        {
            Num w = 1;
            Num omega_m = omegas[i];
            int m = 1 << i, twice_m = m << 1;
            
            for (int k = 0; k < n; k += twice_m)
            {
                Num t = A[k + m];
                Num u = A[k];
                A[k] = add(u, t);
                A[k + m] = sub(u, t);
            }
            
            for (int j = 1; j < m; j++)
            {
                w = mul(w, omega_m);
                for (int k = j; k < n; k += twice_m)
                {
                    Num t = mul(w, A[k + m]);
                    Num u = A[k];
                    A[k] = add(u, t);
                    A[k + m] = sub(u, t);
                }
            }
        }
    }

    inline void butterfly(Num *A, Num *B, Num *W, int size)
    {
        for (int i = 0; i < size; i++)
        {
            Num t = mul(W[i], B[i]);
            Num u = A[i];
            A[i] = add(u, t);
            B[i] = sub(u, t);
        }
    }

    inline void butterfly_1_excl(Num *A, Num *B, Num *W, int size)
    {
        Num t = *B;
        Num u = *A;
        *A = add(u, t);
        *B = sub(u, t);
        for (int i = 1; i < size; i++)
        {
            t = mul(W[i], B[i]);
            u = A[i];
            A[i] = add(u, t);
            B[i] = sub(u, t);
        }
    }
    
    void butterfly_chunk(Num *A, Num *B, int swapper)
    {
        if (!swapper)
        {
            butterfly(A, B + half_chunk_size, buffer, half_chunk_size);
            butterfly(A + half_chunk_size, B, buffer + half_chunk_size, half_chunk_size);
        }
        else
        {
            butterfly(A, B + half_chunk_size, buffer + half_chunk_size, half_chunk_size);
            butterfly(A + half_chunk_size, B, buffer, half_chunk_size);
        }
    }

    void fft_medium(Num *A, Num *omegas, int n, int level_min, int level_cap)
    {
        for (int i = level_min; i < level_cap; i++)
        {
            Num w = 1;
            Num omega_m = omegas[i];
            int m = 1 << i, twice_m = m << 1;
            int j;

            for (j = 0; j < m; j++)
            {
                buffer[j] = w;
                w = mul(w, omega_m);                
            }
            
            
            for (int k = 0; k < n; k += twice_m)
                butterfly_1_excl(A + k, A + k + m, buffer, m);
        }
    }
    
    void fft_big(Num *A, Num *omegas, int n, int log_n)
    {
        for (int i = log_chunk_size; i < log_n; i++)
        {
            int w = 1;
            Num omega_m = omegas[i];
            int m = 1 << i, twice_m = m << 1;
            
            for (int j = 0; j < m; j += chunk_size)
            {
                int k;
                
                for (k = 0; k < chunk_size; k++)
                {
                    buffer[k] = w;
                    w = mul(w, omega_m);
                }
                
                for (k = j; k < n; k += twice_m)
                    butterfly_chunk(A + k, A + k + m, thue_morse(k >> log_chunk_size));
            }
        }
    }

    void swap_chunk(Num *A)
    {
        memcpy(buffer, A, half_chunk_size * sizeof(Num));
        memcpy(A, A + half_chunk_size, half_chunk_size * sizeof(Num));
        memcpy(A + half_chunk_size, buffer, half_chunk_size * sizeof(Num));
    }

    void swap_chunks(Num *A, int n)
    {
        for (int i = 0; i < n; i += chunk_size)
        {
            if (thue_morse(i >> log_chunk_size))
                swap_chunk(A + i);
        }
    }

    
// tests {{{
    
#ifndef NDEBUG

    void test_types()
    {
        assert(((Num) 0 - 1) > (Num) 0); // assert that Num is unsigned
        assert(((DNum) 0 - 1) > (DNum) 0); // assert that DNum is unsigned
        assert(sizeof(Num) * 8 > S);
        assert(sizeof(DNum) >= 2 * sizeof(Num));
    }
    
    void test_P_is_prime()
    {
        for (int i = 2; i * i < P; i++)
            assert(P % i);
    }

    void test_montgomery()
    {
        for (Num i = 0; i < (1 << 31); i+=1270)
        {
            assert(i % P == montgomery31_into(montgomery31(i)));
            assert(i % P == montgomery31(montgomery31_into(i)));
        }
    }
    
#endif

// tests }}}

    void init()
    {
        if (buffer) return;
        buffer = new Num[chunk_size + optimization_offset] + optimization_offset;
        fill_omegas();
    }
    
// _________________________________   omegas   ____________________________________

// {{{

    static Num pow_mod_p(Num base, int exp)
    {
        Num q;
        
        if (exp == 0)
            return montgomery31_into(1);

        q = pow_mod_p(base, exp / 2);
        q = mul(q, q);
            
        if (exp % 2)
            return mul(q, base);
        else
            return q;
    }


    static void calc_omegas(Num *omegas, Num generator)
    {
        assert(P % (1 << S) == 1);
    
        assert(montgomery31_from(pow_mod_p(generator, (P-1) >> 1)) == P - 1);
        Num w = pow_mod_p(generator, (P - 1) >> S);
        for (int i = S; i; i--)
        {
            omegas[i - 1] = w;
            w = mul(w, w);
        }
        assert(montgomery31_from(omegas[0]) == P - 1);
    }


    static Num invert(Num a)
    {
        return pow_mod_p(a, P - 2); // using Fermat Little Theorem
        // (that's very stupid, because Euclid's algorithm is the way to go)
        // but this function is called just once, so no one cares. 
    }
    
    
    void fill_omegas()
    {
        uint32_t m_root = montgomery31_into(primitive_root);
        calc_omegas(positive_omegas, m_root);
        calc_omegas(negative_omegas, invert(m_root));
        
        for (int i = 0; i < S; i++)
            assert(mul(positive_omegas[i], negative_omegas[i]) == montgomery31_into(1));
    }

// }}}

    
public:

    Fourier(int _threshold, int _log_chunk_size) : 
        threshold(_threshold),
        log_chunk_size(_log_chunk_size),
        half_chunk_size(1 << (log_chunk_size - 1)),
        chunk_size(1 << log_chunk_size),
        buffer(NULL)
    {
    }

    ~Fourier()
    {
        if (buffer)
            delete [] (buffer - optimization_offset);
    }


    /*static void polish(Num *A, int n)
    {
        for (int i = 0; i < n; i++)
        {
            int a = convert_2_to_1(A[i]);
            if (a == P) a = 0;
            A[i] = a;
        }
    }*/
    
    
    void fft(Num *out, Num *in, int log_n, int sign)
    {
        init();
        int i;
        int n = 1 << log_n;
        cobra(out, in, log_n);
        for (i = 0; i < n; i++)
            out[i] = montgomery31_into(out[i]);
        Num *omegas = (sign == 1 ? positive_omegas : negative_omegas);
        if (log_n <= log_chunk_size)
        {
            // XXX: log_n <= threshold?
            fft_raw(out, omegas, n, log_n);
        }
        else
        {
            fft_raw(out, omegas, n, threshold);
            fft_medium(out, omegas, n, threshold, log_chunk_size);
            swap_chunks(out, n);
            fft_big(out, omegas, n, log_n);
            swap_chunks(out, n);
            //polish(out, 1 << log_n);
        }
        for (i = 0; i < n; i++)
            out[i] = montgomery31_from(out[i]);
    }

#ifndef NDEBUG
    /** "Slow Fourier Transform".
     * Just for testing.
     * Out-of-place only.
     */
    void sft(Num *out, Num *in, int log_n, int sign)
    {
        init();
        int n = 1 << log_n;
        Num *omegas = (sign == 1 ? positive_omegas : negative_omegas);
        
        memset(out, 0, n * sizeof(Num));
        Num w = omegas[log_n - 1];
        
        for (int i = 0; i < n; i++)
        {
            int j;

            for (j = 0; j < n; j++)
                out[i] = add(out[i], ((DNum) in[j] * pow_mod_p(w, i * j)) % P);
        }

        //polish(out, n);
    }

    void test_all()
    {
        printf("running tests...");
        fflush(stdout);
        test_types();
        test_P_is_prime();
        test_montgomery();
        printf("done\n");
    }

#endif
};


static Fourier<FFT_P, MAX_LOG_FFT_SIZE, uint32_t, uint64_t, /* primary root: */31>
//static Fourier<257, 8, uint32_t, uint64_t, /* primary root: */3>
    fourier(3, /* log_chunk_size: */ 14);

void fft(uint32_t *out, uint32_t *in, int log_n, int sign)
{
    fourier.fft(out, in, log_n, sign);
}

#ifndef NDEBUG
void sft(uint32_t *out, uint32_t *in, int log_n, int sign)
{
    fourier.sft(out, in, log_n, sign);
}

void fft_test()
{
    fourier.test_all();
}


static uint32_t pow_mod_2_32(uint32_t base, uint32_t exp)
{
    uint32_t q;
    
    if (exp == 0)
        return 1;

    q = pow_mod_2_32(base, exp / 2);
    q *= q;
        
    if (exp % 2)
        return q * base;
    else
        return q;
}

uint32_t invert_fft_prime()
{
    return pow_mod_2_32(FFT_P, (1U<<31) - 1);
}

#endif
