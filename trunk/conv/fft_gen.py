M = 25                       # max. tile size
P = ((1<<30) + (1<<25) + 1)  # big prime number

def factorize(n):
    res = []
    for i in xrange(2,n):
        while (n % i == 0):
            n /= i;
            res += [i];
        if n == 1:
            return res
    return res


def pow_modulo(base, exp, m):
    if exp == 0:
        return 1

    q = pow_modulo(base, exp / 2, m);
    q = q * q % m;
        
    if exp % 2:
        return q * base % m
    else:
        return q
    

def is_primitive_root(p, g):
    last_divisor = 1
    factors = factorize(p-1)
    for i in factors:
        if i != last_divisor:
            if pow_modulo(g, (p-1)/i, p) == 1:
                return 0
            last_divisor = i
    return 1



def primitive_root(p):
    for i in xrange(2, p):
        if is_primitive_root(p, i):
            return i;
    raise "this can't happen with prime numbers!"

if P % (1 << M) != 1:
    raise "P mod 2**M should be 1"

G = primitive_root(P)

print "#define P %d" % P
print "static fft_int omega[MAX_LOG_TILE_SIZE] = {"
