#ifndef PROCGEN_IMAGE_H
#define PROCGEN_IMAGE_H

typedef struct ImggenParams
{
	int width, height;
	int rangeMin, rangeMax;

	int hashX, hashY, hashZ;
	int shrStart;
	int iterations;
} ImggenParams;

enum { IMGGEN_BLOB, IMGGEN_GRID, IMGGEN_CLOUDS };

void generateImage(int imggenId, ImggenParams *params, unsigned char *imgPtr);

ImggenParams generateImageParamsDefault(int width, int height, int rangeMin, int rangeMax);
ImggenParams generateImageParamsCloud(int width, int height, int rangeMin, int rangeMax, int hashX, int hashY, int hashZ, int shrStart, int iterations);

#endif
