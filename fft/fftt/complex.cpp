#include <complex>
#include "fftt.h"

using namespace std;

template <class Num> struct ComplexArith
{
    static inline int get_max_log_fft_size()
    {
        return sizeof(unsigned) * 8;
    }

    static inline complex<Num> get_last_positive_omega()
    {
        return polar<Num>(1, 3.1415926535897932 / (1 << (get_max_log_fft_size() - 1)));
    }

    static inline complex<Num> get_last_negative_omega()
    {
        return conj<Num>(get_last_positive_omega());
    }

    static inline complex<Num> add(complex<Num> a, complex<Num> b) { return a + b; }
    static inline complex<Num> sub(complex<Num> a, complex<Num> b) { return a - b; }
    static inline complex<Num> mul(complex<Num> a, complex<Num> b) { return a * b; }
    
    static inline complex<Num> preprocess (complex<Num> a) {return a;}
    static inline complex<Num> postprocess(complex<Num> a) {return a;}

    static inline complex<Num> unity() {return complex<Num>(1);}
};

FFTT<complex<float>, ComplexArith<float> > fft_complex_float(2, 13);
FFTT<complex<double>, ComplexArith<double> > fft_complex_double(2, 12);

#ifdef BENCH

#define LOG_N 20
complex<double> buf[1 << LOG_N];
int main()
{
    int i;
    for (i = 0; i < (1 << LOG_N); i++)
        buf[i] = rand();// % ModPrimeArith::P;
    for (i = 0; i < 10; i++)
        fft_complex_double.fft(buf, buf, LOG_N);
    return 0;
}

#endif
