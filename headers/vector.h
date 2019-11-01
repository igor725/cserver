#ifndef VECTOR_H
#define VECTOR_H
typedef struct _Svec {
	int16_t x, y, z;
} SVec;

typedef struct _Vec {
	float x, y, z;
} Vec;

typedef struct _Ang {
	float yaw, pitch;
} Ang;

static inline bool SVec_Compare(const SVec* a, const SVec* b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

#define Vec_Set(v, x, y, z) ((v).x = x; (v).y = y; (v).z = z;)
#define Vec_IsInvalid(v) ((v)->x == -1 && (v)->y == -1 && (v)->z == -1)
#define Vec_IsZero(v) ((v)->x == 0 && (v)->y == 0 && (v)->z == 0)
#endif