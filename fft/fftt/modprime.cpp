#include <stdint.h>
#include "fftt.h"
#include <stdlib.h>
#include <signal.h>


struct ModPrimeArith
{
    enum
    {
        FFT_P_PLUS = 31,
        FFT_P_MINUS = 27,
        P = ((1U<<FFT_P_PLUS) - (1<<FFT_P_MINUS) + 1),
        G = 31        
    };

    static inline int get_max_log_fft_size()
    {
        return FFT_P_MINUS;
    }

    static uint32_t pow_mod_p(uint32_t base, int exp)
    {
        uint32_t q;
        
        if (exp == 0)
            return preprocess(1);

        q = pow_mod_p(base, exp / 2);
        q = mul(q, q);
            
        if (exp % 2)
            return mul(q, base);
        else
            return q;
    }

    static uint32_t invert(uint32_t a)
    {
        return pow_mod_p(a, P - 2); // using Fermat Little Theorem
        // (that's very stupid, because Euclid's algorithm is the way to go)
        // but this function is called just once, so no one cares. 
    }

    static inline uint32_t get_last_positive_omega()
    {
        return pow_mod_p(preprocess(G), (P - 1) >> FFT_P_MINUS);
    }

    static inline uint32_t get_last_negative_omega()
    {
        return pow_mod_p(invert(preprocess(G)), (P - 1) >> FFT_P_MINUS);
    }
    
    static inline uint32_t add(uint32_t a, uint32_t b)
    {
        uint32_t s = a + b;
        return (s >= P  ?  s - P  :  s);
        //int32_t s = a + b - P;
        //return s + (P & (s >> 31)); 
    }

    static inline uint32_t sub(uint32_t a, uint32_t b)
    {
        int32_t d = a - b;
        return d + ((d >> 31) & P);
        //uint32_t d = P + a - b;
        //return (d >= P  ?  d - P  :  d);
    } 

    /* Montgomery reduction.
     * I've read about this trick in an article by Tommy F\"arnqvist.
     * 
     * x < 2^31 * P
     * result < P, equal to x / 2^31 (mod P)
     */
    static inline uint32_t montgomery31(uint64_t x)
    {
        // x == 2^{31} * x1 + x0
        uint32_t x0 = (uint32_t) x & 0x7FFFFFFF;
        uint32_t x1 = (uint32_t) (x >> 31);

        // 2^{FFT_P_MINUS} + 1 is inverse to P modulo 2^{31}:
        // (1 - 2^{FFT_P_MINUS}) (1 + 2^{FFT_P_MINUS)) = 1 - (...)^2 = 1
        //
        // Here we set `t' to be x0 * P^{-1} modulo 2^{31}.
        // So we'll have  t * P == x (mod 2^{31}).
        //Num t = ((x0 << FFT_P_MINUS) + x0) & 0x7FFFFFFF;
        uint32_t t = (x0 * ((1 << FFT_P_MINUS) + 1)) & 0x7FFFFFFF;

        // u = t * P / 2^{31} 
        // Note that t * P % 2^{31} == x0 by construction of t.
        // So we can put it this way: (t * P - x0) / 2^{31}, where `/' has no remainder.
        uint32_t u = ((uint64_t) t * P) >> FFT_P_PLUS;
        
        /*int32_t d = x1 - u;
        return d + (P & (d >> 31));*/
        if (x1 >= u)
            return x1 - u;
        else
            return P + x1 - u;
    }
    
    
    static inline uint32_t preprocess(uint32_t x)
    {
        return ((uint64_t) x << 31) % P;
    }
    
    static inline uint32_t postprocess(uint32_t x)
    {
        return montgomery31(x);
    }        
    
    static inline uint32_t unity()
    {
        return preprocess(1);
    }
    
    static inline uint32_t mul(uint32_t a, uint32_t b)
    {
        return montgomery31((uint64_t) a * b);
    }

    static inline uint32_t sample(unsigned i)
    {
        return (3 * i * (i >> 4) + 57) % P;
    }
    
    static inline bool eq(uint32_t a, uint32_t b)
    {
        return a == b;
    }
};

//FFTT<uint32_t, ModPrimeArith> fft_mod_prime(2, 14);
FFTT<uint32_t, ModPrimeArith> fft_mod_prime_good(1, 14);
FFTT<uint32_t, ModPrimeArith> fft_mod_prime_bad(1, 3);

#ifdef TEST

/** We're catching SIGABRT in order to get a meaningful coverage
 * profile in such a case.
 */
void aborted(int)
{
    fputs("Abort signal caught, exiting\n", stderr);
    exit(1);
}

int main()
{
    signal(SIGABRT, aborted);
    fft_mod_prime_good.test_bit_reverse();
    fft_mod_prime_good.test_sft_eq_fft_simple();
    fft_mod_prime_bad.test_bit_reverse();
    fft_mod_prime_bad.test_sft_eq_fft_simple();
    fft_mod_prime_good.test_fft_big();
    fft_mod_prime_bad.test_fft_big();
    fft_mod_prime_bad.test_fft_eq_fft_simple();
    fft_mod_prime_good.test_fft_eq_fft_simple();
}

#endif

#ifdef PRIME_BENCH

#define LOG_N 20
uint32_t buf[1 << LOG_N];
int main()
{
    int i;
    for (i = 0; i < (1 << LOG_N); i++)
        buf[i] = rand() % ModPrimeArith::P;
    for (i = 0; i < 10; i++)
        fft_mod_prime_good.fft(buf, buf, LOG_N);
    return 0;
}


#endif
