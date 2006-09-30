#include <stdint.h>
#include <stdlib.h>
#include "fftt.h"

template <class Num> struct RingArith
{
    enum
    {
        PHI = (Num) 1 << (sizeof(Num) * 8 - 1)
    };
    
    static Num pow(Num base, int exp)
    {
        Num q;
        
        if (exp == 0)
            return 1;

        q = pow(base, exp / 2);
        q = q * q;
            
        if (exp % 2)
            return q * base;
        else
            return q;
    }    

    static inline int get_max_log_fft_size()
    {
        return sizeof(Num) * 8 - 2;
    }
    
    static inline Num get_last_positive_omega()
    {
        return 3;
    }


    static inline Num get_last_negative_omega()
    {
        return pow(3, PHI - 2);;
    }

    static inline Num add(Num a, Num b) { return a + b; }
    static inline Num sub(Num a, Num b) { return a - b; }
    static inline Num mul(Num a, Num b) { return a * b; }
    
    static inline Num preprocess (Num a) {return a;}
    static inline Num postprocess(Num a) {return a;}

    static inline Num unity() {return 1;}
};

FFTT<uint32_t, RingArith<uint32_t> > fft_ring_32(2, 14);
FFTT<uint64_t, RingArith<uint64_t> > fft_ring_64(2, 13);
