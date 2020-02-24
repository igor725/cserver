#include "core.h"
#include "vector.h"
#include "csmath.h"
#include <math.h>

#ifndef __GNUC__
float Math_SqrtF(float x) { return sqrtf(x); }
#endif

float Math_Distance(Vec* v1, Vec* v2) {
	float dx = (v1->x - v2->x),
	dy = (v1->y - v2->y),
	dz = (v1->z - v2->z);
	return Math_SqrtF((dx * dx) + (dy * dy) + (dz * dz));
}

void Math_Normalize(Vec* src, Vec* dst) {
	float len = Math_SqrtF((src->x * src->x) + (src->y * src->y) + (src->z * src->z));
	dst->x = src->x / len;
	dst->y = src->y / len;
	dst->z = src->z / len;
}
