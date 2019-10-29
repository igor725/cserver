#ifndef MATH_H
#define MATH_H
typedef uint64_t RNGState;

API void Random_Seed(RNGState* rnd, int32_t seed);
API void Random_SeedFromTime(RNGState* rnd);
API int32_t Random_Next(RNGState* rnd, int32_t n);
API float Random_Float(RNGState* rnd);
API int32_t Random_Range(RNGState* rnd, int32_t min, int32_t max);
#endif
