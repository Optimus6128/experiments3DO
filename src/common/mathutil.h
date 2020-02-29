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

#define PI 3.14159265359f
#define DEG256RAD ((2 * PI) / 256.0f)


typedef struct Vertex
{
	int x, y, z;
}Vertex;

typedef struct Point2D
{
    int x, y;
}Point2D;

extern int shr[257];

int getRand(int from, int to);
void initMathUtil(void);

#endif
