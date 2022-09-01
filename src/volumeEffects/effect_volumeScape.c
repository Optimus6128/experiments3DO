#include "core.h"

#include "effect_volumeScape.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"
#include "file_utils.h"

#define FOV 48
#define VIS_FAR 96
#define VIS_NEAR 8
#define VIS_VER_STEPS (VIS_FAR - VIS_NEAR + 1)
#define VIS_HOR_STEPS SCREEN_WIDTH
#define VIS_SCALE 1

#define HMAP_WIDTH 1024
#define HMAP_HEIGHT 1024
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)


static uint8 *hmap;
//static uint8 *cmap;
static Sprite *testSpr;

static CCB *columnCels;
static uint8 *columnPixels;
static uint16 columnPal[32];

static Vector3D viewPos;
static Vector3D viewAngle;
static Point2D *viewNearPoints;
static Point2D *viewFarPoints;
static int *raySamples;


static void renderScape()
{
	int i,j,l;
	const int playerHeight = viewPos.y;
	int *rs = raySamples;
	uint8 *hm = &hmap[viewPos.z * HMAP_WIDTH + viewPos.x];
	uint8 *dst = columnPixels;

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		int yMax = -SCREEN_HEIGHT/2;
		for (i=0; i<VIS_VER_STEPS; ++i) {
			const int k = *rs++;
			const uint8 hv = *(hm + k);
			int h = ((playerHeight - (int)hv) * 256) / (VIS_NEAR + i);

			CLAMP(h, -SCREEN_HEIGHT/2, SCREEN_HEIGHT/2-1)
			if (yMax < h) {
				for (l=yMax; l<h; ++l) {
					*(dst + l + SCREEN_HEIGHT/2) = 31 - (hv >> 2);
				}
				yMax = h;
			}
			//*(hm + k) = 31;
			//*(dst + i) = hv >> 3;
		}
		setCelWidth(yMax + SCREEN_HEIGHT/2, &columnCels[j]);
		dst += SCREEN_HEIGHT;
	}
}

static void traverseHorizontally(int yawL, int yawR, int z, Point2D *dstPoints)
{
	int i;
	Point2D pl, pr, dHor;

	setPoint2D(&pl, isin[(yawL+64)&255]*z, isin[yawL]*z);
	setPoint2D(&pr, isin[(yawR+64)&255]*z, isin[yawR]*z);

	setPoint2D(&dHor, (pr.x - pl.x) / (VIS_HOR_STEPS - 1), (pr.y - pl.y) / (VIS_HOR_STEPS - 1));

	for (i=0; i<VIS_HOR_STEPS; ++i) {
		dstPoints[i].x = pl.x;
		dstPoints[i].y = pl.y;
		pl.x += dHor.x;
		pl.y += dHor.y;
	}
}

static void initRaySamplePoints()
{
	int i,j,k=0;

	const int halfFov = FOV / 2;
	const int yaw = viewAngle.y & 255;
	const int yawL = (yaw - halfFov) & 255;
	const int yawR = (yaw + halfFov) & 255;

	traverseHorizontally(yawL, yawR, VIS_SCALE * VIS_NEAR, viewNearPoints);
	traverseHorizontally(yawL, yawR, VIS_SCALE * VIS_FAR, viewFarPoints);

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		Point2D *pNear = &viewNearPoints[j];
		Point2D *pFar = &viewFarPoints[j];
		Point2D dVer;

		setPoint2D(&dVer, (pFar->x - pNear->x) / (VIS_VER_STEPS - 1), (pFar->y - pNear->y) / (VIS_VER_STEPS - 1));
		for (i=0; i<VIS_VER_STEPS; ++i) {
			raySamples[k++] = (pNear->y >> FP_BASE) * HMAP_WIDTH + (pNear->x >> FP_BASE);
			pNear->x += dVer.x;
			pNear->y += dVer.y;
		}
	}
}

static void initColumnCels()
{
	int i;

	columnCels = createCels(SCREEN_HEIGHT, 1, 8, CEL_TYPE_CODED, SCREEN_WIDTH);
	setPalGradient(0,31, 0,0,0, 31,31,31, columnPal);

	for (i=0; i<SCREEN_WIDTH; ++i) {
		CCB *cel = &columnCels[i];

		setupCelData(columnPal, &columnPixels[i * SCREEN_HEIGHT], cel);
		setCelPosition(i, SCREEN_HEIGHT-1, cel);
		rotateCelOrientation(cel);
		flipCelOrientation(true, false, cel);

		if (i>0) linkCel(&columnCels[i-1], cel);
	}
}

void effectVolumeScapeInit()
{
	// alloc various tables
	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	columnPixels = (uint8*)AllocMem(SCREEN_WIDTH * SCREEN_HEIGHT, MEMTYPE_ANY);
	raySamples = (int*)AllocMem(VIS_VER_STEPS * VIS_HOR_STEPS * sizeof(int), MEMTYPE_ANY);
	viewNearPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_ANY);
	viewFarPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_ANY);

	// load heightmap
	readBytesFromFileAndStore("data/hmap1.bin", 0, HMAP_SIZE, hmap);
	testSpr = newSprite(HMAP_WIDTH, HMAP_HEIGHT, 8, CEL_TYPE_UNCODED, NULL, hmap);
	setSpritePositionZoom(testSpr, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 64);

	setVector3D(&viewPos, HMAP_WIDTH/4, 0, HMAP_HEIGHT/4);
	setVector3D(&viewAngle, 0,0,0);

	initColumnCels();
	initEngineLUTs();
	initRaySamplePoints();
}

static void updateFromInput()
{
	const int velX = 8;
	const int velY = 8;
	const int velZ = 8;

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		viewPos.x += velX;
	}
	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		viewPos.x -= velX;
	}
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		viewPos.z -= velZ;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		viewPos.z += velZ;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		viewAngle.y += velY;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		viewAngle.y -= velY;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		viewPos.y += velY;
	}

	if (isJoyButtonPressed(JOY_BUTTON_C)) {
		viewPos.y -= velY;
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
	}
}

void effectVolumeScapeRun()
{
	updateFromInput();

	renderScape();

	drawCels(&columnCels[0]);
	//drawSprite(testSpr);
}
