#ifndef MATH_H
#define MATH_H
typedef cs_uint64 RNGState;

API void Random_Seed(RNGState* rnd, cs_int32 seed);
API void Random_SeedFromTime(RNGState* rnd);
API cs_int32 Random_Next(RNGState* rnd, cs_int32 n);
API float Random_Float(RNGState* rnd);
API cs_int32 Random_Range(RNGState* rnd, cs_int32 min, cs_int32 max);
#endif
