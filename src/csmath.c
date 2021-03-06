#include "core.h"
#include "csmath.h"
#include "platform.h"

cs_float Math_Sqrt(cs_float f) {
	union {
		cs_int32 i;
		cs_float f;
	} u;

	u.f = f;
	u.i = (1 << 29) + (u.i >> 1) - (1 << 22);

	u.f = u.f + f / u.f;
	u.f = 0.25f * u.f + f / u.f;

	return u.f;
}

#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_Seed(RNGState *seed, cs_int32 seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

void Random_SeedFromTime(RNGState *secrnd) {
	Random_Seed(secrnd, (cs_int32)Time_GetMSec());
}

cs_int32 Random_Next(RNGState *seed, cs_int32 n) {
	cs_int64 raw;
	cs_int32 bits, val;

	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		raw   = (cs_int64)(*seed >> (48 - 31));
		return (cs_int32)((n * raw) >> 31);
	}

	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits  = (cs_int32)(*seed >> (48 - 31));
		val   = bits % n;
	} while (bits - val + (n - 1) < 0);

	return val;
}

cs_float Random_Float(RNGState *seed) {
	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	cs_int32 raw   = (cs_int32)(*seed >> (48 - 24));
	return raw / ((cs_float)(1 << 24));
}

cs_int32 Random_Range(RNGState *rnd, cs_int32 min, cs_int32 max) {
	return min + Random_Next(rnd, max + 1 - min);
}
