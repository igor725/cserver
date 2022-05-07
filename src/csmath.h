#ifndef CSMATH_H
#define CSMATH_H
#include "core.h"

#define Math_Sq(N) ((N) * (N))

typedef cs_uint64 RNGState;
API void Random_Seed(RNGState *rnd, cs_int32 seed);
API void Random_SeedFromTime(RNGState *rnd);
API cs_int32 Random_Next(RNGState *rnd, cs_int32 n);
API cs_float Random_Float(RNGState *rnd);
API cs_int32 Random_Range(RNGState *rnd, cs_int32 min, cs_int32 max);
API cs_float Math_Sqrt(cs_float f);
#endif
