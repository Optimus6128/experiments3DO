#ifndef PROCGEN_IMAGE_H
#define PROCGEN_IMAGE_H

typedef struct ImggenParams
{
	int hashX, hashY, hashZ;
	int shrStart;
	int iterations;
} ImggenParams;

enum { IMGGEN_CLOUDS };

void generateImage(int width, int height, ubyte *imgPtr, int rangeMin, int rangeMax, int imggenId, const ImggenParams params);

ImggenParams generateImageParamsCloud(int hashX, int hashY, int hashZ, int shrStart, int iterations);

#endif
