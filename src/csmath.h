#ifndef CSMATH_H
#define CSMATH_H
#include "vector.h"
#include <math.h>

#ifdef __GNUC__
#define Math_SqrtF(x) __builtin_sqrtf(x)
#else
#define Math_SqrtF(x) sqrtf(x)
#endif

typedef cs_uint64 RNGState;

API void Random_Seed(RNGState *rnd, cs_int32 seed);
API cs_int32 Random_Next(RNGState *rnd, cs_int32 n);
API cs_float Random_Float(RNGState *rnd);
API cs_int32 Random_Range(RNGState *rnd, cs_int32 min, cs_int32 max);
API cs_float Math_Distance(Vec *v1, Vec *v2);
API void Math_Normalize(Vec *src, Vec *dst);
#endif
