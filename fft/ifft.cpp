#include "ifft.h"
#include <assert.h>
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
 * primary_root - a primary_root modulo P
 * log_chunk_size - log_2 of the size of a chunk (most likely, should be cache size) 
 */
template <int P, int S, class Num, class DNum, Num primary_root>
class Fourier
{
    const int threshold;
    const int log_chunk_size;
    const int half_chunk_size;
    const int chunk_size;
    Num *buffer;
    Num positive_omegas[S], negative_omegas[S];
    unsigned *permutation_table[S];
 
    enum
    {
        optimization_offset = 16,   // this helps very little
        MAX_PERMUTATION_CACHED = 8
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

    static inline Num mul(Num a, Num b)
    {
        return (DNum) a * b % P;
    }

    // representations and arithmetic modulo P }}}


    void fill_permutation_table_if_needed(int log_n)
    {
        assert(log_n <= S);
        int n = 1 << log_n;
        
        if (permutation_table[log_n])
            return;

        uint32_t *table = permutation_table[log_n] = new uint32_t[1 << log_n];
        
        for (int i = 0; i < n; i++)
            table[i] = reverse_bits(i, log_n);
    }

    
    void out_of_place_permute(Num *out, Num *in, int log_n)
    {
        int n = 1 << log_n;

        // assert that the areas don't overlap
        assert(!(out >= in  &&  out < in + n));
        assert(!(in >= out  &&  in < out + n));
        
        if (log_n <= MAX_PERMUTATION_CACHED)
        {
            fill_permutation_table_if_needed(log_n);
            for (int i = 0; i < n; i++)
                out[permutation_table[log_n][i]] = in[i];
        }
        else
        {
            for (int i = 0; i < n; i++)
                out[reverse_bits(i, log_n)] = in[i];
        }
    }
 
    void in_place_permute(Num *A, int log_n)
    {
        unsigned n = 1 << log_n;
        
        #define PERMUTE(GET_PERMUTATION)                    \
            for (unsigned i = 0; i < n; i++)                \
            {                                               \
                unsigned rev = GET_PERMUTATION(i, log_n);   \
                if (i < rev)                                \
                {                                           \
                    Num tmp = A[i];                         \
                    A[i] = A[rev];                          \
                    A[rev] = tmp;                           \
                }                                           \
            }
        

        if (log_n <= MAX_PERMUTATION_CACHED)
        {
            fill_permutation_table_if_needed(log_n);
            #define PERM_GET_FROM_TABLE(I, LOG_N) permutation_table[LOG_N][I]
            PERMUTE(PERM_GET_FROM_TABLE);
        }
        else
        {
            #define PERM_CALCULATE(I, LOG_N) reverse_bits(I, LOG_N)
            PERMUTE(PERM_CALCULATE);
        }    
    }

    void permute(Num *out, Num *in, int log_n)
    {
        if (in == out)
            in_place_permute(in, log_n);
        else
            out_of_place_permute(out, in, log_n);
    }

    
    /* FFT without bit-reverse.
     * This corresponds to Cormen, Leiserson, Rivest, 32.3, "Iterative-FFT",
     * except that:
     *   1) no Bit-Reverse-Copy is called
     *   2) i == s - 1  (we have i, CLR have s)
     *   3) our twice_m is m in CLR
     */
    static void fft_raw(Num *A, Num *omegas, int n, int levels_to_go)
    {
        for (int i = 0; i < levels_to_go; i++)
        {
            int w = 1;
            Num omega_m = omegas[i];
            int m = 1 << i, twice_m = m << 1;
            
            for (int j = 0; j < m; j++)
            {
                for (int k = j; k < n; k += twice_m)
                {
                    Num t = mul(w, A[k + m]);
                    Num u = A[k];
                    A[k] = add(u, t);
                    A[k + m] = sub(u, t);
                }
                w = mul(w, omega_m);
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
            int w = 1;
            Num omega_m = omegas[i];
            int m = 1 << i, twice_m = m << 1;
            int j;

            for (j = 0; j < m; j++)
            {
                buffer[j] = w;
                w = mul(w, omega_m);                
            }
            
            
            for (int k = 0; k < n; k += twice_m)
                butterfly(A + k, A + k + m, buffer, m);
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
            return 1;

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
    
        Num w = pow_mod_p(generator, (P - 1) >> S);
        for (int i = S; i; i--)
        {
            omegas[i - 1] = w;
            w = mul(w, w);
        }
        assert(omegas[0] == P - 1);
    }


    static Num invert(Num a)
    {
        return pow_mod_p(a, P - 2); // using Fermat Little Theorem
    }
    
    
    void fill_omegas()
    {
        calc_omegas(positive_omegas, primary_root);
        calc_omegas(negative_omegas, invert(primary_root));
        
        for (int i = 0; i < S; i++)
            assert(mul(positive_omegas[i], negative_omegas[i]) == 1);
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
        int n = 1 << log_n;
        permute(out, in, log_n);
        Num *omegas = (sign == 1 ? positive_omegas : negative_omegas);
        if (log_n <= log_chunk_size)
            fft_raw(out, omegas, n, log_n);
        else
        {
            fft_raw(out, omegas, n, threshold);
            fft_medium(out, omegas, n, threshold, log_chunk_size);
            swap_chunks(out, n);
            fft_big(out, omegas, n, log_n);
            swap_chunks(out, n);
            //polish(out, 1 << log_n);
        }
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
                out[i] = add(out[i], mul(in[j], pow_mod_p(w, i * j)));
        }

        //polish(out, n);
    }
#endif
};


static Fourier<FFT_P, MAX_LOG_FFT_SIZE, uint32_t, uint64_t, /* primary root: */10>
//static Fourier<257, 8, uint32_t, uint64_t, /* primary root: */3>
    fourier(2, /* log_chunk_size: */ 12);

void fft(uint32_t *out, uint32_t *in, int log_n, int sign)
{
    fourier.fft(out, in, log_n, sign);
}

#ifndef NDEBUG
void sft(uint32_t *out, uint32_t *in, int log_n, int sign)
{
    fourier.sft(out, in, log_n, sign);
}
#endif

