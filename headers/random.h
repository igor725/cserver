#ifndef MATH_H
#define MATH_H
typedef uint64_t RNGState;

API void Random_Seed(RNGState* rnd, int seed);
API void Random_SeedFromTime(void);
API int Random_Next(RNGState* rnd, int n);
API float Random_Float(RNGState* rnd);
API int Random_Range(RNGState* rnd, int min, int max);
#endif
