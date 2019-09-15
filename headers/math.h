#ifndef MATH_H
#define MATH_H
typedef uint64 RNGState;

API void Random_Seed(RNGState* rnd, int seed);
API int Random_Next(RNGState* rnd, int n);
API float Random_Float(RNGState* rnd);
API int Random_Range(RNGState* rnd, int min, int max);
#endif
