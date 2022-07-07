#include "stdlib.h"

#include "mathutil.h"

#include "core.h"


int isin[256];
int recZ[NUM_REC_Z];


int isqrt(int x) {
    int q = 1, r = 0;
    while (q <= x) {
        q <<= 2;
    }
    while (q > 1) {
        int t;
        q >>= 2;
        t = x - r - q;
        r >>= 1;
        if (t >= 0) {
            x = t;
            r += q;
        }
    }
    return r;
} 

int getRand(int from, int to)
{
	int rnd;
	if (from > to) return 0;
	if (from==to) return to;
	rnd = from + (rand() % (to - from));
	return rnd;
}

int getShr(int n)
{
	int b = -1;
	do{
		b++;
	}while((n>>=1)!=0);
	return b;
}

void initEngineLUTs()
{
	int i;
	for(i=0; i<256; i++) {
		isin[i] = SinF16(i << 16) >> 4;
	}

	for (i=1; i<NUM_REC_Z; ++i) {
		recZ[i] = (1 << REC_FPSHR) / i;
	}
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
