#ifndef VECTOR_H
#define VECTOR_H
typedef struct {
	cs_int16 x, y, z;
} SVec;

typedef struct {
	float x, y, z;
} Vec;

typedef struct {
	float yaw, pitch;
} Ang;

static inline cs_bool SVec_Compare(const SVec* a, const SVec* b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

static inline cs_bool Vec_Compare(const Vec* a, const Vec* b) {
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

static inline cs_bool Ang_Compare(const Ang* a, const Ang* b) {
	return a->yaw == b->yaw && a->pitch == b->pitch;
}

#define Ang_Set(a, ay, ap) (a).yaw = ay; (a).pitch = ap;
#define SVec_Set(v, vx, vy, vz) (v).x = (cs_int16)vx; (v).y = (cs_int16)vy; (v).z = (cs_int16)vz;
#define Vec_Set(v, vx, vy, vz) (v).x = vx; (v).y = vy; (v).z = vz;
#define Vec_IsInvalid(v) ((v)->x == -1 && (v)->y == -1 && (v)->z == -1)
#define Vec_IsZero(v) ((v)->x == 0 && (v)->y == 0 && (v)->z == 0)
#endif // VECTOR_H
