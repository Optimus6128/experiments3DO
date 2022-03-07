#ifndef MATHUTIL_H
#define MATHUTIL_H

#define FP_CORE 16
#define FP_BASE 12
#define FP_BASE_TO_CORE (FP_CORE - FP_BASE)

#define FLOAT_TO_FIXED(f,b) ((int)((f) * (1 << b)))
#define INT_TO_FIXED(i,b) ((i) * (1 << b))
#define UINT_TO_FIXED(i,b) ((i) << b)
#define FIXED_TO_INT(x,b) ((x) >> b)
#define FIXED_TO_FLOAT(x,b) ((float)(x) / (1 << b))
#define FIXED_MUL(x,y,b) (((x) * (y)) >> b)
#define FIXED_DIV(x,y,b) (((x) << b) / (y))
#define FIXED_SQRT(x,b) (sqrt((x) << b))

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

extern int shr[257];

int getRand(int from, int to);
int getShr(int n);
void initMathUtil(void);

void setVector3D(Vector3D *v, int x, int y, int z);
void setVector3DfromVertices(Vector3D *v, Vertex *v0, Vertex *v1);
void calcVector3Dcross(Vector3D *vRes, Vector3D *v0, Vector3D *v1);
void normalizeVector3D(Vector3D *v);

Point2Darray *initPoint2Darray(int numPoints);
void addPoint2D(Point2Darray *ptArray, int x, int y);
void destroyPoint2Darray(Point2Darray *ptArray);

#endif
