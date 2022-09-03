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
#define VIS_NEAR 16
#define VIS_VER_STEPS (VIS_FAR - VIS_NEAR + 1)
#define VIS_HOR_STEPS (SCREEN_WIDTH / 2)

#define V_PLAYER_HEIGHT 64
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HORIZON (3*SCREEN_HEIGHT / 4)

#define HMAP_WIDTH 512
#define HMAP_HEIGHT 512
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)

#define NUM_SHADE_PALS 1


static uint8 *hmap;
static uint8 *cmap;

static CCB *columnCels;
static uint16 *columnPixels;

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
	uint16 *dst = columnPixels;
	const int viewerOffset = viewPos.z * HMAP_WIDTH + viewPos.x;

	uint16 *pmap = (uint16*)&cmap[HMAP_SIZE];	// palette comes after the bitmap data

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		int yMax = 0;
		for (i=0; i<VIS_VER_STEPS; ++i) {
			const int k = *rs++;
			const int mapOffset = (viewerOffset + k) & (HMAP_WIDTH * HMAP_HEIGHT - 1);
			const int h = (((-playerHeight + hmap[mapOffset]) * recZ[VIS_NEAR + i]) >> (REC_FPSHR - V_HEIGHT_SCALER_SHIFT)) + V_HORIZON;

			if (yMax < h) {
				const uint16 cv = pmap[cmap[mapOffset]];
				for (l=yMax; l<h; ++l) {
					*(dst + l) = cv;
				}
				yMax = h;
			}
		}
		if (yMax==0) {
			columnCels[j].ccb_Flags |= CCB_SKIP;
		} else {
			columnCels[j].ccb_Flags &= ~CCB_SKIP;
			setCelWidth(yMax, &columnCels[j]);
		}
		dst += SCREEN_HEIGHT;
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

static void initRaySamplePoints()
{
	int i,j,k=0;

	const int halfFov = FOV / 2;
	const int yaw = viewAngle.y & 255;
	const int yawL = (yaw - halfFov) & 255;
	const int yawR = (yaw + halfFov) & 255;

	traverseHorizontally(yawL, yawR, VIS_NEAR, viewNearPoints);
	traverseHorizontally(yawL, yawR, VIS_FAR, viewFarPoints);

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

	columnCels = createCels(SCREEN_HEIGHT, 1, 16, CEL_TYPE_UNCODED, VIS_HOR_STEPS);

	for (i=0; i<VIS_HOR_STEPS; ++i) {
		CCB *cel = &columnCels[i];

		setupCelData(NULL, &columnPixels[i * SCREEN_HEIGHT], cel);
		setCelPosition(2*i, SCREEN_HEIGHT-1, cel);
		rotateCelOrientation(cel);
		flipCelOrientation(true, false, cel);
		cel->ccb_VDX = 2 << 16;

		if (i>0) linkCel(&columnCels[i-1], cel);
	}
}

void effectVolumeScapeInit()
{
	const int cmapSize = HMAP_SIZE + 256 * NUM_SHADE_PALS * sizeof(uint16);

	// alloc various tables
	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	cmap = AllocMem(cmapSize, MEMTYPE_ANY);
	columnPixels = (uint16*)AllocMem(VIS_HOR_STEPS * SCREEN_HEIGHT * sizeof(uint16), MEMTYPE_ANY);
	raySamples = (int*)AllocMem(VIS_VER_STEPS * VIS_HOR_STEPS * sizeof(int), MEMTYPE_ANY);
	viewNearPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_ANY);
	viewFarPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_ANY);

	// load heightmap and colormap
	readBytesFromFileAndStore("data/hmap1.bin", 0, HMAP_SIZE, hmap);
	readBytesFromFileAndStore("data/cmap1.bin", 0, cmapSize, cmap);

	setVector3D(&viewPos, 3*HMAP_WIDTH/4, V_PLAYER_HEIGHT, HMAP_HEIGHT/6);
	setVector3D(&viewAngle, 0,128,0);

	initColumnCels();
	initEngineLUTs();
	initRaySamplePoints();
}

static void updateFromInput()
{
	const int velX = 4;
	const int velY = 2;
	const int velZ = 4;

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
}
