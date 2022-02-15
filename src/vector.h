#ifndef VECTOR_H
#define VECTOR_H
#include "core.h"

typedef struct _SVec {
	cs_int16 x, y, z;
} SVec;

typedef struct _Vec {
	cs_float x, y, z;
} Vec;

typedef struct _Ang {
	cs_float yaw, pitch;
} Ang;

static inline cs_bool SVec_Compare(const SVec *a, const SVec *b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

static inline cs_bool Vec_Compare(const Vec *a, const Vec *b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

static inline cs_bool Ang_Compare(const Ang *a, const Ang *b) {
	return a->yaw == b->yaw && a->pitch == b->pitch;
}

#define Ang_Set(a, ay, ap) (a).yaw = ay, (a).pitch = ap;
#define Vec_Add(dst, v1, v2) (dst).x = (v1).x + (v2).x, (dst).y = (v1).y + (v2).y, (dst).z = (v1).z + (v2).z
#define Vec_Sub(dst, v1, v2) (dst).x = (v1).x - (v2).x, (dst).y = (v1).y - (v2).y, (dst).z = (v1).z - (v2).z
#define Vec_Mul(dst, v1, v2) (dst).x = (v1).x * (v2).x, (dst).y = (v1).y * (v2).y, (dst).z = (v1).z * (v2).z
#define Vec_Div(dst, v1, v2) (dst).x = (v1).x / (v2).x, (dst).y = (v1).y / (v2).y, (dst).z = (v1).z / (v2).z
#define Vec_Copy(dv, sv) (dv).x = (cs_float)(sv).x, (dv).y = (cs_float)(sv).y, (dv).z = (cs_float)(sv).z
#define SVec_Copy(dv, sv) (dv).x = (cs_int16)(sv).x, (dv).y = (cs_int16)(sv).y, (dv).z = (cs_int16)(sv).z
#define Vec_Set(v, vx, vy, vz) (v).x = vx, (v).y = vy, (v).z = vz
#define Vec_IsInvalid(v) ((v)->x == -1 && (v)->y == -1 && (v)->z == -1)
#define Vec_IsZero(v) ((v).x == 0 && (v).y == 0 && (v).z == 0)
#define Vec_HaveZero(v) ((v).x == 0 || (v).y == 0 || (v).z == 0)
#define Vec_Scale(v, s) (v).x *= s, (v).y *= s, (v).z *= s
#endif // VECTOR_H
