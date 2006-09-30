#ifndef FFTT_H
#define FFTT_H

#include <string.h>
#include <stdio.h>
#include <assert.h>

enum
{
    FFTT_SKIP_PREPROCESS  = 1,
    FFTT_SKIP_POSTPROCESS = 2,
    FFTT_SKIP_BIT_REVERSE = 4,
    FFTT_SIMPLE           = 8
};

template <class Num, class Arith> class FFTT
{
    bool bit_reverse_first;
    int twiddle_precompute_level;
    int log_chunk_size;
    
    unsigned half_chunk_size;
    unsigned chunk_size;
    Num *buffer;
    Num *positive_twiddle_factors;
    Num *negative_twiddle_factors;
    int buffer_offset;
    Num *positive_omegas, *negative_omegas;
    int max_log_fft_size;


    // chunk swapping {{{
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
    // chunk swapping }}}


    // bit-reverse {{{
    
    // Reverse bits (swap 0th with (log_n-1)-th and so on).
    static unsigned reverse_bits(unsigned a, int log_n)
    {
        unsigned result = 0;
        
        for (int i = 0; i < log_n; i++)
        {
            if (a & (1 << i))
                result += 1 << (log_n - i - 1);
        }
        assert(result < (1U << log_n));
        
        return result;
    }
    

    static inline void bit_reverse_load
        (Num *buf, Num *in, unsigned b, int s, int q, unsigned cache_cap)
    {
        for (unsigned a = 0; a < cache_cap; a++)
        {
            unsigned a_rev = reverse_bits(a, q);
            for (unsigned c = 0; c < cache_cap; c++)
                buf[(a_rev << q) + c] = in[(a << s) + (b << q) + c];
        }
    }

    static inline void bit_reverse_save
        (Num *buf, Num *out, unsigned b_rev, int s, int q, unsigned cache_cap)
    {
        for (unsigned c = 0; c < cache_cap; c++)
        {
            unsigned c_rev = reverse_bits(c, q);
            for (unsigned a = 0; a < cache_cap; a++)
                out[(c_rev << s) + (b_rev << q) + a] = buf[(a << q) + c];
        }
    }

    

    /** \brief Bit-reverse (both in-place and out-of-place).
     * This is COBRA ("Cache-Optimal Bit-Reverse Algorithm")
     * from the paper "Towards an Optimal Bit-Reversal Permutation Program"
     * by Larry Carter and Kang Su Gatlin.
     */
    void out_of_place_bit_reverse(Num *out, Num *in, int log_n)
    {
        // Choosing q
        int q1 = log_n / 2;
        int q2 = log_chunk_size / 2;
        int q = (q1 > q2  ?  q2  :  q1);
        
        // COBRA begins
        unsigned cache_cap = 1U << q;
        int b_len = log_n - q - q;
        int s = log_n - q;
        unsigned b_cap = 1U << b_len;
        
        assert(b_len >= 0);
        assert(2 * q <= log_chunk_size);
        
        for (unsigned b = 0; b < b_cap; b++)
        {
            unsigned b_rev = reverse_bits(b, b_len);
            bit_reverse_load(buffer, in,  b,     s, q, cache_cap);
            bit_reverse_save(buffer, out, b_rev, s, q, cache_cap);    
        }
    }

    void in_place_bit_reverse(Num *A, int log_n)
    {
        // Choosing q
        int q1 = log_n / 2;
        int q2 = (log_chunk_size - 1) / 2; // we use 2 buffers of 2^2q each
        int q = (q1 > q2  ?  q2  :  q1);
        Num *buffer2 = buffer + half_chunk_size; 

        // COBRA begins
        unsigned cache_cap = 1U << q;
        int b_len = log_n - q - q;
        int s = log_n - q;
        unsigned b_cap = 1U << b_len;
        
        assert(b_len >= 0);
        assert(2 * q <= log_chunk_size);
        
        for (unsigned b = 0; b < b_cap; b++)
        {
            unsigned b_rev = reverse_bits(b, b_len);
            if (b_rev > b) continue;
            if (b_rev == b)
            {
                bit_reverse_load(buffer, A, b, s, q, cache_cap);
                bit_reverse_save(buffer, A, b, s, q, cache_cap);
            }
            else
            {                
                bit_reverse_load(buffer,  A, b,     s, q, cache_cap);
                bit_reverse_load(buffer2, A, b_rev, s, q, cache_cap);
                bit_reverse_save(buffer,  A, b_rev, s, q, cache_cap);
                bit_reverse_save(buffer2, A, b,     s, q, cache_cap);
            }
        }
    }
    
    void bit_reverse(Num *out, Num *in, int log_n)
    {
        if (out == in)
            in_place_bit_reverse(out, log_n);
        else
        {
            // assert that the regions don't overlap
            assert(in < out || in >= out + (1 << log_n));
            assert(out < in || out >= in + (1 << log_n));
            
            out_of_place_bit_reverse(out, in, log_n);
        }
    }
    
    // bit-reverse }}}
    
    
    // butterflies {{{
    
    static inline void one_butterfly_without_twiddle(Num *A, Num *B)
    {
        Num u = *A;
        Num t = *B;
        *A = Arith::add(u, t);
        *B = Arith::sub(u, t);
    }
    

    static inline void one_butterfly_ascending(Num *A, Num *B, Num twiddle)
    {
        Num u = *A;
        Num t = Arith::mul(twiddle, *B);
        *A = Arith::add(u, t);
        *B = Arith::sub(u, t);
    }

    static inline void one_butterfly_descending(Num *A, Num *B, Num twiddle)
    {
        Num u = *A;
        Num t = *B;
        *A = Arith::add(u, t);
        *B = Arith::mul(twiddle, Arith::sub(u, t));
    }

    inline void butterfly_ascending(Num *A, Num *B, Num *W, unsigned size)
    {
        for (unsigned i = 0; i < size; i++)
            one_butterfly_ascending(A++, B++, *W++);
    }

    inline void butterfly_descending(Num *A, Num *B, Num *W, unsigned size)
    {
        for (unsigned i = 0; i < size; i++)
            one_butterfly_descending(A++, B++, *W++);
    }

    
    #define BUTTERFLY_CHUNK(X) void butterfly_chunk_##X(Num *A, Num *B, int swapper) \
    { \
        if (!swapper) \
        { \
            butterfly_##X(A, B + half_chunk_size, buffer, half_chunk_size); \
            butterfly_##X(A + half_chunk_size, B, buffer + half_chunk_size, half_chunk_size); \
        } \
        else \
        { \
            butterfly_##X(A, B + half_chunk_size, buffer + half_chunk_size, half_chunk_size); \
            butterfly_##X(A + half_chunk_size, B, buffer, half_chunk_size); \
        } \
    } 
    BUTTERFLY_CHUNK(ascending);
    BUTTERFLY_CHUNK(descending);

    // butterflies }}}
    
    // fft levels {{{
    
    static void fft_level_0(Num *A, unsigned n)
    {
        for (unsigned k = 0; k < n; k += 2)
            one_butterfly_without_twiddle(A + k, A + k + 1);
    }


    void fft_small(Num *A, Num *omegas, unsigned n, int level)
    {
        Num w = Arith::unity();
        Num omega_m = omegas[level];
        unsigned m = 1U << level, twice_m = m << 1;
        unsigned k;

        for (k = 0; k < n; k += twice_m)
            one_butterfly_without_twiddle(A + k, A + k + m);

        if (bit_reverse_first)
        {
            for (unsigned j = 1; j < m; j++)
            {
                w = Arith::mul(w, omega_m);
                for (k = j; k < n; k += twice_m)
                    one_butterfly_ascending(A + k, A + k + m, w);
            }
        }
        else
        {
            for (unsigned j = 1; j < m; j++)
            {
                w = Arith::mul(w, omega_m);
                for (k = j; k < n; k += twice_m)
                    one_butterfly_descending(A + k, A + k + m, w);
            }            
        }
    }
    

    void fft_medium(Num *A, Num *twiddle_buf, unsigned n, int level)
    {
        unsigned m = 1U << level, twice_m = m << 1;
        Num *tf = twiddle_buf + m;
        
        if (bit_reverse_first)
        {
            for (unsigned k = 0; k < n; k += twice_m)
                butterfly_ascending(A + k, A + k + m, tf, m);
        }
        else
        {
            for (unsigned k = 0; k < n; k += twice_m)
                butterfly_descending(A + k, A + k + m, tf, m);
        }
    }


    void fft_not_big(Num *A, Num *omegas, Num *twiddle_buf, unsigned n, int level)
    {
        if (!level)
            fft_level_0(A, n);
        else if (level < twiddle_precompute_level)
            fft_small(A, omegas, n, level);
        else
            fft_medium(A, twiddle_buf, n, level);
    }

    
    void fft_big(Num *A, Num *omegas, unsigned n, int level)
    {
        Num w = Arith::unity();
        Num omega_m = omegas[level];
        unsigned m = 1 << level, twice_m = m << 1;
        
        for (unsigned j = 0; j < m; j += chunk_size)
        {
            unsigned k;
            
            for (k = 0; k < chunk_size; k++)
            {
                buffer[k] = w;
                w = Arith::mul(w, omega_m);
            }
            
            if (bit_reverse_first)
            {
                for (k = j; k < n; k += twice_m)
                    butterfly_chunk_ascending(A + k, A + k + m, thue_morse(k >> log_chunk_size));
            }
            else
            {
                for (k = j; k < n; k += twice_m)
                    butterfly_chunk_descending(A + k, A + k + m, thue_morse(k >> log_chunk_size));
            }
        }
    }

    // fft levels }}}

    

    // initialization {{{

    void fill_omegas(Num *omegas, Num w)
    {
        for (int i = max_log_fft_size; i; i--)
        {
            omegas[i - 1] = w;
            w = Arith::mul(w, w);
        }
    }
    
    
    void compute_twiddle_factors(Num *buf, Num *omegas, int level)
    {
        assert(level < log_chunk_size);
        unsigned m = 1 << level;
        Num *tf = buf + m;
        Num omega_m = omegas[level];
        Num w = Arith::unity();
        for (unsigned j = 0; j < m; j++)
        {
            tf[j] = w;
            w = Arith::mul(w, omega_m); 
        }
    }    


    void init()
    {
        if (buffer) return;
        buffer = new Num[chunk_size + buffer_offset] + buffer_offset;
        positive_twiddle_factors = new Num[chunk_size];
        negative_twiddle_factors = new Num[chunk_size];
        max_log_fft_size = Arith::get_max_log_fft_size();
        positive_omegas = new Num[max_log_fft_size];
        negative_omegas = new Num[max_log_fft_size];
        fill_omegas(positive_omegas, Arith::get_last_positive_omega());
        fill_omegas(negative_omegas, Arith::get_last_negative_omega());

        for (int level = 0; level < log_chunk_size; level++)
        {
            compute_twiddle_factors(positive_twiddle_factors, positive_omegas, level);
            compute_twiddle_factors(negative_twiddle_factors, negative_omegas, level);
        }
    }

    // initialization }}}

    void fft_raw_ascending(Num *A, Num *omegas, Num *twiddle_buf, int n, int log_n, bool simple)
    {
        if (simple)
        {
            for (int level = 0; level < log_n; level++)
                fft_small(A, omegas, n, level);
            return;
        }
        
        int level_cap = log_n > log_chunk_size ? log_chunk_size : log_n;
        
        for (int level = 0; level < level_cap; level++)
            fft_not_big(A, omegas, twiddle_buf, n, level);
        
        if (log_n > log_chunk_size)
        {
            swap_chunks(A, n);
            for (int level = log_chunk_size; level < log_n; level++)
                fft_big(A, omegas, n, level);
            swap_chunks(A, n);
        }
    }

    void fft_raw_descending(Num *A, Num *omegas, Num *twiddle_buf, int n, int log_n, bool simple)
    {
        if (simple)
        {
            for (int level = log_n; level; level--)
                fft_small(A, omegas, n, level - 1);
            return;
        }
        
        int level_cap = log_n > log_chunk_size ? log_chunk_size : log_n;
        
        if (log_n > log_chunk_size)
        {
            swap_chunks(A, n);
            for (int level = log_n - 1; level >= log_chunk_size; level--)
                fft_big(A, omegas, n, level);
            swap_chunks(A, n);
        }
        
        for (int level = level_cap; level; level--)
            fft_not_big(A, omegas, twiddle_buf, n, level - 1);
    }


    
public:
    
    FFTT(int _twiddle_precompute_level,
         int _log_chunk_size,
         int _bit_reverse_first = true,
         int _buffer_offset = 128 / sizeof(Num)):
        bit_reverse_first(_bit_reverse_first),
        twiddle_precompute_level(_twiddle_precompute_level),
        log_chunk_size(_log_chunk_size),
        half_chunk_size(1U << (_log_chunk_size - 1)),
        chunk_size(1U << _log_chunk_size),
        buffer(0),
        buffer_offset(_buffer_offset)
    {
    }

    ~FFTT()
    {
        if (buffer)
        {
            delete [] (buffer - buffer_offset);
            delete [] positive_twiddle_factors;
            delete [] negative_twiddle_factors;
            delete [] positive_omegas;
            delete [] negative_omegas;
        }
    }


    void fft(Num *out, Num *in, int log_n, int sign = 1, int skip_flags = 0)
    {
        init();
        unsigned n = 1U << log_n;
        Num *omegas = (sign == 1 ? positive_omegas : negative_omegas);
        Num *twiddle_buf = (sign == 1 ? positive_twiddle_factors : negative_twiddle_factors);

        if (bit_reverse_first)
        {
            if (!(skip_flags & FFTT_SKIP_BIT_REVERSE))
                bit_reverse(out, in, log_n);
            else if (out != in)
                memcpy(out, in, n * sizeof(Num));
            
            if (!(skip_flags & FFTT_SKIP_PREPROCESS))
            {
                for (unsigned i = 0; i < n; i++)
                    out[i] = Arith::preprocess(out[i]);
            }
            
            fft_raw_ascending(out, omegas, twiddle_buf, n, log_n, skip_flags & FFTT_SIMPLE);
            
            if (!(skip_flags & FFTT_SKIP_POSTPROCESS))
            {
                for (unsigned i = 0; i < n; i++)
                    out[i] = Arith::postprocess(out[i]);
            }
        }
        else
        {
            if (out != in)
                memcpy(out, in, n * sizeof(Num));
            
            if (!(skip_flags & FFTT_SKIP_PREPROCESS))
            {
                for (unsigned i = 0; i < n; i++)
                    out[i] = Arith::preprocess(out[i]);
            }
            
            fft_raw_descending(out, omegas, twiddle_buf, n, log_n, skip_flags & FFTT_SIMPLE);
            
            if (!(skip_flags & FFTT_SKIP_POSTPROCESS))
            {
                for (unsigned i = 0; i < n; i++)
                    out[i] = Arith::postprocess(out[i]);
            }

            if (!(skip_flags & FFTT_SKIP_BIT_REVERSE))
                bit_reverse(out, out, log_n);
        }
    }

    #ifndef NDEBUG

private:

    Num power(Num base, int exp)
    {
        Num q;
        if (exp == 0)
            return Arith::unity();

        q = power(base, exp / 2);
        q = Arith::mul(q, q);

        if (exp % 2)
            return Arith::mul(q, base);
        else
            return q;
    }

    // out-of-place only
    // (and input is overwritten!)
    void sft(Num *out, Num *in, int log_n, int sign = 1, int skip_flags = 0)
    {
        init();
        assert(out != in);
        if (!log_n)
        {
            *out = *in;
            return;
        }
        
        assert(log_n <= max_log_fft_size);
        unsigned n = 1U << log_n;
        Num *omegas = (sign == 1 ? positive_omegas : negative_omegas);
        Num zero = Arith::sub(Arith::unity(), Arith::unity()); // argh!
        
        if (!(skip_flags & FFTT_SKIP_PREPROCESS))
        {
            for (unsigned i = 0; i < n; i++)
                in[i] = Arith::preprocess(in[i]);
        }

        for (unsigned i = 0; i < n; i++)
            out[i] = zero;
        
        Num w = omegas[log_n - 1];
        
        for (unsigned i = 0; i < n; i++)
            for (unsigned j = 0; j < n; j++)
                out[i] = Arith::add(out[i], Arith::mul(in[j], power(w, i * j)));
                
        if (!(skip_flags & FFTT_SKIP_POSTPROCESS))
        {
            for (unsigned i = 0; i < n; i++)
                out[i] = Arith::postprocess(out[i]);
        }
    }

public:

    
    void test_bit_reverse()
    {
        init();
        for (int i = 0; i < 18; i++)
        {
            int n = 1 << i;
            Num *buf = new Num[1 << i];
            for (int k = 0; k < n; k++)
                buf[k] = k;
            bit_reverse(buf, buf, i);
            for (int k = 0; k < n; k++)
                assert(buf[k] == reverse_bits(k, i));
        }
        puts("test_bit_reverse passed");
    }
    
    
    void test_sft_eq_fft_simple()
    {
        for (int i = 0; i < 8; i++)
        {
            int n = 1 << i;
            Num buf1[n];
            Num buf2[n];
            Num buf3[n];
            for (int k = 0; k < n; k++)
                buf1[k] = buf2[k] = Arith::sample(k);
            sft(buf3, buf1, i); 
            fft(buf2, buf2, i, 1, FFTT_SIMPLE);
            for (int k = 0; k < n; k++)
                assert(Arith::eq(buf2[k], buf3[k]));
        }
        puts("test_sft_eq_fft_simple passed");
    }

    void test_fft_big()
    {
        for (int i = 0; i < 5; i++)
        {
            Num *buf1 = new Num[1 << i];
            Num *buf2 = new Num[1 << i];
            Num *buf3 = new Num[1 << i];
            for (int k = 0; k < (1 << i); k++)
                buf1[k] = buf2[k] = Arith::sample(k);
            swap_chunks(buf2, 1 << i);

            for (int level = log_chunk_size; level < i; level++)
            {
                fft_small(buf1, positive_omegas, 1 << i, level);
                fft_big(buf2, positive_omegas, 1 << i, level);
                memcpy(buf3, buf2, (1 << i) * sizeof(Num));
                swap_chunks(buf3, 1 << i);
                for (int k = 0; k < (1 << i); k++)
                    assert(Arith::eq(buf1[k], buf3[k]));
            }
            delete [] buf1;
            delete [] buf2;
            delete [] buf3;
        }        
        puts("test_fft_big passed");
    }

    void test_fft_eq_fft_simple()
    {
        for (int i = 0; i < 18; i++)
        {
            Num *buf1 = new Num[1 << i];
            Num *buf2 = new Num[1 << i];
            for (int k = 0; k < (1 << i); k++)
                buf1[k] = Arith::sample(k);
            fft(buf2, buf1, i); 
            fft(buf1, buf1, i, 1, FFTT_SIMPLE);
            for (int k = 0; k < (1 << i); k++)
                assert(Arith::eq(buf1[k], buf2[k]));
            delete [] buf1;
            delete [] buf2;
        }
        puts("test_fft_eq_fft_simple passed");
    }
    
    #endif // !NDEBUG
};

#endif // FFTT_H
