#include "stdlib.h"

#include "mathutil.h"

#include "core.h"


int shr[257]; // ugly way to get precalced fast right shift for division with power of two numbers

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

void initMathUtil()
{
	int i;
	for (i=1; i<=256; i++)
	{
		shr[i] = getShr(i);
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
