#ifndef CSMATH_H
#define CSMATH_H
#include "core.h"
#include "vector.h"

typedef cs_uint64 RNGState;
API void Random_Seed(RNGState *rnd, cs_int32 seed);
API void Random_SeedFromTime(RNGState *rnd);
API cs_int32 Random_Next(RNGState *rnd, cs_int32 n);
API cs_float Random_Float(RNGState *rnd);
API cs_int32 Random_Range(RNGState *rnd, cs_int32 min, cs_int32 max);
API cs_float Math_Sqrt(const cs_float f);
API cs_float Math_Distance(Vec *v1, Vec *v2);
API void Math_Normalize(Vec *src, Vec *dst);
#endif
