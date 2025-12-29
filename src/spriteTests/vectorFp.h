#ifndef VECTOR_FP_H
#define VECTOR_FP_H

#include "mathutil.h"

#define RP_BITS 12

#define FLT_TO_FP(f)    FLOAT_TO_FIXED(f,RP_BITS)
#define INT_TO_FP(i)    INT_TO_FIXED(i,RP_BITS)
#define FP_TO_INT(x)    FIXED_TO_INT(x,RP_BITS)
#define FP_TO_FLT(x)    FIXED_TO_FLOAT(x,RP_BITS)
#define FP_MUL(x,y)     FIXED_MUL(x,y,RP_BITS)
#define FP_MUL_64(x,y)  FIXED_MUL_64(x,y,RP_BITS)
#define FP_DIV(x,y)     FIXED_DIV(x,y,RP_BITS)
#define FP_DIV_64(x,y)  FIXED_DIV_64(x,y,RP_BITS)
#define FP_SQRT(x)		FIXED_SQRT(x,RP_BITS)

#define VEC_INT_TO_FP(x,y,z) makeVec3fpI(INT_TO_FP(x), INT_TO_FP(y), INT_TO_FP(z))
#define VEC_FLT_TO_FP(x,y,z) makeVec3fpI(FLT_TO_FP(x), FLT_TO_FP(y), FLT_TO_FP(z))

#define FP_MUL_T(x,y) FP_MUL(x,y)
#define FP_DIV_T(x,y) FP_DIV(x,y)


typedef struct Vec3fp
{
    int x, y, z;
} Vec3fp;

Vec3fp makeVec3fpF(float x, float y, float z)
{
	Vec3fp v;

	v.x = FLT_TO_FP(x);
	v.y = FLT_TO_FP(y);
	v.z = FLT_TO_FP(z);

	return v;
}

Vec3fp makeVec3fpI(int x, int y, int z)
{
	Vec3fp v;

	v.x = x;
	v.y = y;
	v.z = z;

	return v;
}

Vec3fp makeVec3fpZ(void)
{
	Vec3fp v;

	v.x = 0;
	v.y = 0;
	v.z = 0;

	return v;
}

void operator_add(Vec3fp* v, Vec3fp *dst) {
	dst->x += v->x;
	dst->y += v->y;
	dst->z += v->z;
}

void operator_sub(Vec3fp* v, Vec3fp *dst) {
	dst->x -= v->x;
	dst->y -= v->y;
	dst->z -= v->z;
}

void operator_mul(int s, Vec3fp *dst) {
	dst->x = FP_MUL(dst->x,s);
	dst->y = FP_MUL(dst->y,s);
	dst->z = FP_MUL(dst->z,s);
}

void operator_div(int d, Vec3fp *dst) {
	dst->x = FP_DIV(dst->x, d);
	dst->y = FP_DIV(dst->y, d);
	dst->z = FP_DIV(dst->z, d);
}

#endif
