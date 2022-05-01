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

static INL cs_bool SVec_Compare(const SVec *a, const SVec *b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

static INL cs_bool Vec_Compare(const Vec *a, const Vec *b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

static INL cs_bool Ang_Compare(const Ang *a, const Ang *b) {
	return a->yaw == b->yaw && a->pitch == b->pitch;
}

#define Ang_Set(a, ay, ap) (a).yaw = ay, (a).pitch = ap;
#define Vec_Add(d, v1, v2) (d).x = (v1).x + (v2).x, (d).y = (v1).y + (v2).y, (d).z = (v1).z + (v2).z
#define Vec_Sub(d, v1, v2) (d).x = (v1).x - (v2).x, (d).y = (v1).y - (v2).y, (d).z = (v1).z - (v2).z
#define Vec_Mul(d, v1, v2) (d).x = (v1).x * (v2).x, (d).y = (v1).y * (v2).y, (d).z = (v1).z * (v2).z
#define Vec_Div(d, v1, v2) (d).x = (v1).x / (v2).x, (d).y = (v1).y / (v2).y, (d).z = (v1).z / (v2).z
#define Vec_Min(d, v1, v2) (d).x = min((v1).x, (v2).x), (d).y = min((v1).y, (v2).y), (d).z = min((v1).z, (v2).z)
#define Vec_Max(d, v1, v2) (d).x = max((v1).x, (v2).x), (d).y = max((v1).y, (v2).y), (d).z = max((v1).z, (v2).z)
#define Vec_Cross(d, v1, v2) (d).x = (v1).y * (v2).z - (v1).z * (v2).y, (d).y = (v1).z * (v2).x - (v1).x * (v2).z, \
	(d).z = (v1).x * (v2).y - (v1).y * (v2).x
#define Vec_DivN(v, n) (v).x /= n, (v).y /= n, (v).z /= n
#define Vec_Copy(dv, sv) (dv).x = (cs_float)(sv).x, (dv).y = (cs_float)(sv).y, (dv).z = (cs_float)(sv).z
#define SVec_Copy(dv, sv) (dv).x = (cs_int16)(sv).x, (dv).y = (cs_int16)(sv).y, (dv).z = (cs_int16)(sv).z
#define Vec_Set(v, vx, vy, vz) (v).x = vx, (v).y = vy, (v).z = vz
#define Vec_IsNegative(v) ((v).x < 0 || (v).y < 0 || (v).z < 0)
#define Vec_IsZero(v) ((v).x == 0 && (v).y == 0 && (v).z == 0)
#define Vec_HaveZero(v) ((v).x == 0 || (v).y == 0 || (v).z == 0)
#define Vec_Scale(v, s) (v).x *= s, (v).y *= s, (v).z *= s
#endif // VECTOR_H
