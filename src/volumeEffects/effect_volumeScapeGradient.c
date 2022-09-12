#include "core.h"

#include "effect_volumeScapeGradient.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"
#include "file_utils.h"

#define FP_LINEAR 10

#define FOV 48
#define VIS_FAR 176

#define VIS_NEAR 16
#define VIS_VER_STEPS ((VIS_FAR - VIS_NEAR) / 2)
#define VIS_HOR_STEPS (SCREEN_WIDTH / 2)

#define V_PLAYER_HEIGHT 176
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HEIGHT_SCALER (1 << V_HEIGHT_SCALER_SHIFT)
#define V_HORIZON (3*SCREEN_HEIGHT / 4)

#define HMAP_WIDTH 512
#define HMAP_HEIGHT 512
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)


static int lintab[VIS_VER_STEPS];
static int heightScaleTab[VIS_VER_STEPS];

static uint8 *hmap;

static CCB *columnCels;
static uint8 *columnPixels;
static uint16 columnPal[32];

static Vector3D viewPos;
static Vector3D viewAngle;
static int *raySamples;

static bool walk = true;


static void renderScape()
{
	int i,j;
	const int playerHeight = viewPos.y;
	int *rs = raySamples;
	const int viewerOffset = viewPos.z * HMAP_WIDTH + viewPos.x;

	uint8 *dstBase = columnPixels;
	for (j=0; j<VIS_HOR_STEPS; ++j) {
		int yMax = 0;
		uint8 *dst = dstBase;

		for (i=0; i<VIS_VER_STEPS; ++i) {
			const int mapOffset = (viewerOffset + *(rs+i)) & (HMAP_SIZE - 1);
			const int hm = hmap[mapOffset];
			int h = (((-playerHeight + hm) * heightScaleTab[i]) >> (REC_FPSHR - V_HEIGHT_SCALER_SHIFT)) + V_HORIZON;
			if (h > SCREEN_HEIGHT-1) h = SCREEN_HEIGHT-1;

			if (yMax < h) {
				const uint8 cv = hm >> 2;

				int hCount = h-yMax;

				do {
					*dst++ = cv;
				}while(--hCount > 0);

				/*
				// Failed attempt to sometimes draw bytes, other times 4 bytes at once. Column diffs are so small most of the time there is barely any gain.
				if (hCount < 8) {
					do {
						*dst++ = cv;
					}while(--hCount > 0);
				} else {
					int xlp = yMax & 3;
					if (xlp) {
						xlp = 4 - xlp;
						while (xlp-- > 0 && hCount-- > 0) {
							*dst++ = cv;
						}
					}

					{
						uint32 *dst32 = (uint32*)dst;
						uint32 cv32;
						if (hCount >= 4) cv32 = cv * 0x01010101;
						while(hCount >= 4) {
							*dst32++ = cv32;
							hCount-=4;
						};

						dst = (uint8*)dst32;
						while (hCount-- > 0) {
							*dst++ = cv;
						}
					}
				}
				*/

				yMax = h;
				if (yMax == SCREEN_HEIGHT - 1) break;
			}
		}
		rs += VIS_VER_STEPS;

		if (yMax==0) {
			columnCels[j].ccb_Flags |= CCB_SKIP;
		} else {
			columnCels[j].ccb_Flags &= ~CCB_SKIP;
			setCelWidth(yMax, &columnCels[j]);
		}
		dstBase += SCREEN_HEIGHT;
	}

	drawCels(&columnCels[0]);
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


static void createLinearTable()
{
	int i;
	float zone = 0;
	float dz = 1.0f / (VIS_VER_STEPS - 1);
	for (i = 0; i < VIS_VER_STEPS; ++i) {
		float inter = zone; //pow(zone, 1.0f);
		lintab[i] = (int)(inter * (1 << FP_LINEAR));
		heightScaleTab[i] = recZ[VIS_NEAR + ((lintab[i] * VIS_FAR) >> FP_LINEAR)];
		zone += dz;
	}
}

static void initRaySamplePoints()
{
	int i,j,k=0;

	const int halfFov = FOV / 2;
	const int yaw = viewAngle.y & 255;
	const int yawL = (yaw - halfFov) & 255;
	const int yawR = (yaw + halfFov) & 255;
	const int icos = isin[halfFov + 64];

	Point2D *viewNearPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_TRACKSIZE);
	Point2D *viewFarPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_TRACKSIZE);

	traverseHorizontally(yawL, yawR, (VIS_NEAR << FP_BASE) / icos, viewNearPoints);
	traverseHorizontally(yawL, yawR, (VIS_FAR << FP_BASE) / icos, viewFarPoints);

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		Point2D pInter;
		Point2D *pNear = &viewNearPoints[j];
		Point2D *pFar = &viewFarPoints[j];

		for (i=0; i<VIS_VER_STEPS; ++i) {
			pInter.x = pNear->x + (((pFar->x - pNear->x) * lintab[i]) >> FP_LINEAR);
			pInter.y = pNear->y + (((pFar->y - pNear->y) * lintab[i]) >> FP_LINEAR);
			raySamples[k++] = (pInter.y >> FP_BASE) * HMAP_WIDTH + (pInter.x >> FP_BASE);
		}
	}

	FreeMem(viewNearPoints, -1);
	FreeMem(viewFarPoints, -1);
}

static void initColumnCels()
{
	int i;

	columnCels = createCels(SCREEN_HEIGHT, 1, 8, CEL_TYPE_CODED, VIS_HOR_STEPS);

	setPalGradient(0,31, 1,2,4, 31,16,12 , columnPal);

	for (i=0; i<VIS_HOR_STEPS; ++i) {
		CCB *cel = &columnCels[i];

		setupCelData(columnPal, &columnPixels[i * SCREEN_HEIGHT], cel);
		setCelPosition(2*i, SCREEN_HEIGHT-1, cel);
		rotateCelOrientation(cel);
		flipCelOrientation(true, false, cel);
		cel->ccb_VDX = 2 << 16;

		if (i>0) linkCel(&columnCels[i-1], cel);
	}
}


void effectVolumeScapeGradientInit()
{
	// alloc various tables
	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	columnPixels = (uint8*)AllocMem(VIS_HOR_STEPS * SCREEN_HEIGHT, MEMTYPE_ANY);
	raySamples = (int*)AllocMem(VIS_VER_STEPS * VIS_HOR_STEPS * sizeof(int), MEMTYPE_ANY);

	// load heightmap and colormap
	readBytesFromFileAndStore("data/hmap1.bin", 0, HMAP_SIZE, hmap);

	setVector3D(&viewPos, 3*HMAP_WIDTH/4, V_PLAYER_HEIGHT, HMAP_HEIGHT/6);
	setVector3D(&viewAngle, 0,128,0);

	initColumnCels();
	initEngineLUTs();

	createLinearTable();
	initRaySamplePoints();
}

static void updateFromInput()
{
	const int velX = 3;
	const int velY = 2;
	const int velZ = 3;

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		viewPos.x -= velX;
	}
	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		viewPos.x += velX;
	}
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		viewPos.z += velZ;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		viewPos.z -= velZ;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		viewPos.y += velY;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		viewPos.y -= velY;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_SELECT)) {
		walk = !walk;
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
	}

	if (walk) {
		viewPos.y = hmap[(viewPos.z * HMAP_WIDTH + viewPos.x) & (HMAP_WIDTH * HMAP_HEIGHT - 1)] + V_PLAYER_HEIGHT/4;
	}
}

void effectVolumeScapeGradientRun()
{
	updateFromInput();

	renderScape();
}
