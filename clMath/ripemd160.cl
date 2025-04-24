#ifndef _RIPEMD160_CL
#define _RIPEMD160_CL


__constant unsigned int _RIPEMD160_IV[5] = {
    0x67452301,
    0xefcdab89,
    0x98badcfe,
    0x10325476,
    0xc3d2e1f0
};

__constant unsigned int _K0 = 0x5a827999;
__constant unsigned int _K1 = 0x6ed9eba1;
__constant unsigned int _K2 = 0x8f1bbcdc;
__constant unsigned int _K3 = 0xa953fd4e;

__constant unsigned int _K4 = 0x7a6d76e9;
__constant unsigned int _K5 = 0x6d703ef3;
__constant unsigned int _K6 = 0x5c4dd124;
__constant unsigned int _K7 = 0x50a28be6;

#define rotl(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define F(x, y, z) ((x) ^ (y) ^ (z))

#define G(x, y, z) (((x) & (y)) | (~(x) & (z)))

#define H(x, y, z) (((x) | ~(y)) ^ (z))

#define I(x, y, z) (((x) & (z)) | ((y) & ~(z)))

#define J(x, y, z) ((x) ^ ((y) | ~(z)))

#define FF(a, b, c, d, e, m, s)\
    a += (F((b), (c), (d)) + (m));\
    a = (rotl((a), (s)) + (e));\
    c = rotl((c), 10)

#define GG(a, b, c, d, e, x, s)\
    a += G((b), (c), (d)) + (x) + _K0;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define HH(a, b, c, d, e, x, s)\
    a += H((b), (c), (d)) + (x) + _K1;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define II(a, b, c, d, e, x, s)\
    a += I((b), (c), (d)) + (x) + _K2;\
    a = rotl((a), (s)) + e;\
    c = rotl((c), 10)

#define JJ(a, b, c, d, e, x, s)\
    a += J((b), (c), (d)) + (x) + _K3;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define FFF(a, b, c, d, e, x, s)\
    a += F((b), (c), (d)) + (x);\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define GGG(a, b, c, d, e, x, s)\
    a += G((b), (c), (d)) + x + _K4;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define HHH(a, b, c, d, e, x, s)\
    a += H((b), (c), (d)) + (x) + _K5;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define III(a, b, c, d, e, x, s)\
    a += I((b), (c), (d)) + (x) + _K6;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#define JJJ(a, b, c, d, e, x, s)\
    a += J((b), (c), (d)) + (x) + _K7;\
    a = rotl((a), (s)) + (e);\
    c = rotl((c), 10)

#endif
