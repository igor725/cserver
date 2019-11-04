#include "core.h"
#include "random.h"
#include "platform.h"

#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_Seed(RNGState* seed, cs_int32 seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

void Random_SeedFromTime(RNGState* secrnd) {
	Random_Seed(secrnd, (cs_int32)Time_GetMSec());
}

cs_int32 Random_Next(RNGState* seed, cs_int32 n) {
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

float Random_Float(RNGState* seed) {
	cs_int32 raw;

	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	raw   = (cs_int32)(*seed >> (48 - 24));
	return raw / ((float)(1 << 24));
}

cs_int32 Random_Range(RNGState* rnd, cs_int32 min, cs_int32 max) {
	return min + Random_Next(rnd, max - min);
}
