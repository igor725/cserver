#include "core.h"
#include "random.h"

#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_Seed(RNGState* seed, int seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

void Random_SeedFromTime(void) {
	Random_Seed(&secrnd, (int)Time_GetMSec());
}

int Random_Next(RNGState* seed, int n) {
	int64_t raw;
	int bits, val;

	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		raw   = (int64_t)(*seed >> (48 - 31));
		return (int)((n * raw) >> 31);
	}

	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits  = (int)(*seed >> (48 - 31));
		val   = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

float Random_Float(RNGState* seed) {
	int raw;

	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	raw   = (int)(*seed >> (48 - 24));
	return raw / ((float)(1 << 24));
}

int Random_Range(RNGState* rnd, int min, int max) {
	return min + Random_Next(rnd, max - min);
}
