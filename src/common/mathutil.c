#include "stdlib.h"

#include "mathutil.h"

#define SQRT_LUT_SIZE 16384

int *isin;
int *recZ;

static int sqrtLUT[SQRT_LUT_SIZE];


static int isqrtCalc(int x) {
    long long int q = 1;	// very high numbers over ((1<<30)-1) will freeze in while if this wasn't 64bit
	int r = 0;
    while (q <= x) {
        q <<= 2;
    }

    while (q > 1) {
        int t;
        q >>= 2;
        t = (int)(x - r - q);
        r >>= 1;
        if (t >= 0) {
            x = t;
            r += (int)q;
        }
    }
    return r;
} 

int isqrt(int x) {
	if (x < SQRT_LUT_SIZE) {
		return sqrtLUT[x];
	} else {
		return isqrtCalc(x);
	}
}

int getRand(int from, int to)
{
	int rnd;
	if (from > to) return 0;
	if (from==to) return to;
	rnd = from + (rand() % (to - from));
	return rnd;
}

int getShr(unsigned int n)
{
	int b = -1;
	do{
		b++;
	}while((n>>=1)!=0);
	return b;
}

void initSQRTLut()
{
	int i;
	for (i=0; i<SQRT_LUT_SIZE; ++i) {
		sqrtLUT[i] = isqrtCalc(i);
	}
}

void initEngineLUTs()
{
	int i;

	isin = (int*)AllocMem(256 * sizeof(int), MEMTYPE_ANY);
	recZ = (int*)AllocMem(NUM_REC_Z * sizeof(int), MEMTYPE_ANY);

	for(i=0; i<256; i++) {
		isin[i] = SinF16(i << 16) >> FP_BASE_TO_CORE;
	}

	for (i=1; i<NUM_REC_Z; ++i) {
		recZ[i] = (1 << REC_FPSHR) / i;
	}

	initSQRTLut();
}

void setVector3D(Vector3D *v, int x, int y, int z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}

void copyVector3D(Vector3D *src, Vector3D *dst)
{
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}

void setVector3DfromVertices(Vector3D *v, Vertex *v0, Vertex *v1)
{
	v->x = v1->x - v0->x;
	v->y = v1->y - v0->y;
	v->z = v1->z - v0->z;
}

void addVector3D(Vector3D *dst, Vector3D *src1, Vector3D *src2)
{
	dst->x = src1->x + src2->x;
	dst->y = src1->y + src2->y;
	dst->z = src1->z + src2->z;
}

void subVector3D(Vector3D *dst, Vector3D *src1, Vector3D *src2)
{
	dst->x = src1->x - src2->x;
	dst->y = src1->y - src2->y;
	dst->z = src1->z - src2->z;
}

void mulScalarVector3D(Vector3D *vec, int m)
{
	vec->x *= m;
	vec->y *= m;
	vec->z *= m;
}

void divScalarVector3D(Vector3D *vec, int d)
{
	vec->x /= d;
	vec->y /= d;
	vec->z /= d;
}

void calcVector3Dcross(Vector3D *vRes, Vector3D *v0, Vector3D *v1)
{
	vRes->x = v0->y * v1->z - v0->z * v1->y;
	vRes->y = v0->z * v1->x - v0->x * v1->z;
	vRes->z = v0->x * v1->y - v0->y * v1->x;
}

int getVector3Ddot(Vector3D *v0, Vector3D *v1)
{
	return v0->x*v1->x + v0->y*v1->y + v0->z*v1->z;
}

int getVector3Dlength(Vector3D *v)
{
	return isqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

void normalizeVector3D(Vector3D *v)
{
	const int length = getVector3Dlength(v);

	if (length != 0) {
		v->x = (v->x << NORMAL_SHIFT) / length;
		v->y = (v->y << NORMAL_SHIFT) / length;
		v->z = (v->z << NORMAL_SHIFT) / length;
	}
}

void setPoint2D(Point2D *pt, int x, int y)
{
	pt->x = x;
	pt->y = y;
}

Point2Darray *initPoint2Darray(int numPoints)
{
	Point2Darray *ptArray = (Point2Darray*)AllocMem(sizeof(Point2Darray), MEMTYPE_ANY);

	ptArray->pointsNum = numPoints;
	ptArray->currentIndex = 0;
	ptArray->points = (Point2D*)AllocMem(sizeof(Point2D) * numPoints, MEMTYPE_TRACKSIZE);

	return ptArray;
}

void addPoint2D(Point2Darray *ptArray, int x, int y)
{
	if (ptArray->currentIndex < ptArray->pointsNum) {
		Point2D *newPoint = &ptArray->points[ptArray->currentIndex++];
		newPoint->x = x;
		newPoint->y = y;
	}
}

void destroyPoint2Darray(Point2Darray *ptArray)
{
	FreeMem(ptArray->points, -1);
	FreeMem(ptArray, sizeof(Point2Darray));
}

void SoftMulVec3Mat33_F16(vec3f16 *dest, vec3f16 *src, mat33f16 mat)
{
	frac16 *m = (frac16*)mat;
	frac16 *s = (frac16*)src;
	frac16 *d = (frac16*)dest;

	const int x = *s++;
	const int y = *s++;
	const int z = *s++;

	*d++ = FIXED_TO_INT(x * m[0] + y * m[3] + z * m[6], FP_CORE);
	*d++ = FIXED_TO_INT(x * m[1] + y * m[4] + z * m[7], FP_CORE);
	*d++ = FIXED_TO_INT(x * m[2] + y * m[5] + z * m[8], FP_CORE);
}

void SoftMulManyVec3Mat33_F16(vec3f16* dest, vec3f16* src, mat33f16 mat, int32 count)
{
	int i;
	frac16 *m = (frac16*)mat;
	frac16 *s = (frac16*)src;
	frac16 *d = (frac16*)dest;

	for (i = 0; i < count; ++i)
	{
		const int x = *s++;
		const int y = *s++;
		const int z = *s++;

		*d++ = FIXED_TO_INT(x * m[0] + y * m[3] + z * m[6], FP_CORE);
		*d++ = FIXED_TO_INT(x * m[1] + y * m[4] + z * m[7], FP_CORE);
		*d++ = FIXED_TO_INT(x * m[2] + y * m[5] + z * m[8], FP_CORE);
	}
}
