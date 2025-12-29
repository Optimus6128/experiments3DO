#ifndef MATHUTIL_H
#define MATHUTIL_H

#include "core.h"

#define FP_CORE 16
#define FP_BASE 12
#define FP_BASE_TO_CORE (FP_CORE - FP_BASE)

#define NORMAL_SHIFT 8

#define FLOAT_TO_FIXED(f,b) ((int)((f) * (1 << b)))
#define INT_TO_FIXED(i,b) ((i) << b)
#define UINT_TO_FIXED(i,b) ((i) << b)
#define FIXED_TO_INT(x,b) ((x) >> b)
#define FIXED_TO_FLOAT(x,b) ((float)(x) / (1 << b))
#define FIXED_MUL(x,y,b) (((x) * (y)) >> b)
#define FIXED_MUL_64(x,y,b) ((int)(((long long int)(x) * (long long int)(y)) >> b))
#define FIXED_DIV(x,y,b) (((x) << b) / (y))
#define FIXED_DIV_64(x,y,b) (((long long int)(x) << b) / (y))
#define FIXED_SQRT(x,b) (isqrt(x) << (b / 2))

#define VEC3D_TO_FIXED(v,b) v.x *= (1 << b); v.y *= (1 << b); v.z *= (1 << b);

#define PI 3.14159265359f
#define DEG256RAD ((2 * PI) / 256.0f)


typedef struct Vertex
{
	int x,y,z;
}Vertex;

typedef struct Vector3D
{
	int x,y,z;
}Vector3D;

typedef struct BoundingBox
{
	Vector3D center;
	Vector3D halfSize;
	int diagonalLength;
}BoundingBox;

typedef struct Point2D
{
    int x,y;
}Point2D;

typedef struct Point2Darray
{
	Point2D *points;
	int currentIndex;
	int pointsNum;
}Point2Darray;


#define NUM_REC_Z 16384
#define REC_FPSHR 18

extern int *isin;
extern int *recZ;

int isqrt(int x);

int getRand(int from, int to);
int getShr(unsigned int n);

void initEngineLUTs(void);
void initSQRTLut(void);

void setVector3D(Vector3D *v, int x, int y, int z);
void copyVector3D(Vector3D *src, Vector3D *dst);
void setVector3DfromVertices(Vector3D *v, Vertex *v0, Vertex *v1);
void calcVector3Dcross(Vector3D *vRes, Vector3D *v0, Vector3D *v1);
int getVector3Ddot(Vector3D *v0, Vector3D *v1);
int getVector3Dlength(Vector3D *v);
void normalizeVector3D(Vector3D *v);

void addVector3D(Vector3D *dst, Vector3D *src1, Vector3D *src2);
void subVector3D(Vector3D *dst, Vector3D *src1, Vector3D *src2);
void mulScalarVector3D(Vector3D *vec, int m);
void divScalarVector3D(Vector3D *vec, int d);

void setPoint2D(Point2D *pt, int x, int y);
Point2Darray *initPoint2Darray(int numPoints);
void addPoint2D(Point2Darray *ptArray, int x, int y);
void destroyPoint2Darray(Point2Darray *ptArray);

void SoftMulVec3Mat33_F16(vec3f16 *dest, vec3f16 *src, mat33f16 mat);
void SoftMulManyVec3Mat33_F16(vec3f16* dest, vec3f16* src, mat33f16 mat, int32 count);

#endif
