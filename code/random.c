#include "core.h"
#include "random.h"
#include "platform.h"

#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_Seed(RNGState* seed, int32_t seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

void Random_SeedFromTime(RNGState* secrnd) {
	Random_Seed(secrnd, (int32_t)Time_GetMSec());
}

int32_t Random_Next(RNGState* seed, int32_t n) {
	int64_t raw;
	int32_t bits, val;

	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		raw   = (int64_t)(*seed >> (48 - 31));
		return (int32_t)((n * raw) >> 31);
	}

	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits  = (int32_t)(*seed >> (48 - 31));
		val   = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

float Random_Float(RNGState* seed) {
	int32_t raw;

	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	raw   = (int32_t)(*seed >> (48 - 24));
	return raw / ((float)(1 << 24));
}

int32_t Random_Range(RNGState* rnd, int32_t min, int32_t max) {
	return min + Random_Next(rnd, max - min);
}
