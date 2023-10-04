#include "core.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"
#include "file_utils.h"

#define FP_LINEAR 12

#define FOV 48
#define VIS_FAR 160
#define VIS_NEAR 16
#define VIS_SCALE_DOWN 0.375
#define VIS_VER_STEPS ((int)((VIS_FAR - VIS_NEAR) * VIS_SCALE_DOWN))
#define VIS_HOR_STEPS (SCREEN_WIDTH / 2)

#define V_PLAYER_HEIGHT 176
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HEIGHT_SCALER (1 << V_HEIGHT_SCALER_SHIFT)
#define V_HORIZON (3*SCREEN_HEIGHT / 4)

#define HMAP_WIDTH 512
#define HMAP_HEIGHT 512
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)

#define NUM_SHADE_PALS VIS_VER_STEPS

static int lintab[VIS_VER_STEPS];
static int heightScaleTab[VIS_VER_STEPS];

static unsigned char *hmap;
static unsigned char *cmap;

static CCB *columnCels;
static uint16 *columnPixels;

static Vector3D viewPos;
static Vector3D viewAngle;
static Point2D *viewNearPoints;
static Point2D *viewFarPoints;
static int *raySamples;

static bool walk = true;


static void renderScape()
{
	int i,j,l;
	const int playerHeight = viewPos.y;
	int *rs = raySamples;
	uint16 *dst = columnPixels;
	const int viewerOffset = viewPos.z * HMAP_WIDTH + viewPos.x;

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		uint16 *pmap = (uint16*)&cmap[HMAP_SIZE];	// palette comes after the bitmap data
		int yMax = 0;
		int h,mapOffset;
		
		for (i=0; i<VIS_VER_STEPS; ++i) {
			mapOffset = (viewerOffset + *(rs+i)) & (HMAP_WIDTH * HMAP_HEIGHT - 1);
			h = (((-playerHeight + hmap[mapOffset]) * heightScaleTab[i]) >> (REC_FPSHR - V_HEIGHT_SCALER_SHIFT)) + V_HORIZON;

			if (yMax < h) {
				const uint16 cv = pmap[cmap[mapOffset]];
				if (h > SCREEN_HEIGHT-1) h = SCREEN_HEIGHT-1;
				for (l=yMax; l<h; ++l) *(dst + l) = cv;
				yMax = h;
				if (yMax == SCREEN_HEIGHT - 1) break;
			}
			pmap += 256;
		}

		rs += VIS_VER_STEPS;

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

/*static void createNonLinearTablePow()
{
	int i;
	for (i = 0; i < VIS_VER_STEPS; ++i) {
		float zone = (float)i / (VIS_VER_STEPS - 1);
		float inter = pow(zone, 2.0f);
		lintab[i] = (int)(inter * (1 << FP_LINEAR));
		heightScaleTab[i] = recZ[VIS_NEAR + ((lintab[i] * VIS_FAR) >> FP_LINEAR)];
	}
}*/

static void createNonLinearTable()
{
	int i;
	float ii = 0.0f;
	float di = (1.0f / VIS_VER_STEPS) * VIS_SCALE_DOWN;
	for (i = 0; i < VIS_VER_STEPS; ++i) {
		lintab[i] = (int)(ii * (1 << FP_LINEAR));
		heightScaleTab[i] = recZ[VIS_NEAR + ((lintab[i] * VIS_FAR) >> FP_LINEAR)];
		ii += di;
		di *=  1.03f;
		//di += 0.000375f;
		if (ii > 1.0f) ii = 1.0f;
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

static void initShadedPals()
{
	int i,j;

	uint16 *pmap = (uint16*)&cmap[HMAP_SIZE];	// palette comes after the bitmap data
	uint16 *pmapNext = pmap + 256;

	for (j=1; j<NUM_SHADE_PALS; ++j) {
		//int cshade = ((NUM_SHADE_PALS-1 - j) * 256) / (NUM_SHADE_PALS-1);
		int cshade = 288 - ((256 * lintab[j]) >> FP_LINEAR);
		CLAMP(cshade,0,256);
		for (i=0; i<256; ++i) {
			pmapNext[i] = shadeColor(pmap[i], cshade);
		}
		pmapNext += 256;
	}
}

void effectVolumeScapeInit()
{
	const int cmapSize = HMAP_SIZE + 512 * NUM_SHADE_PALS;

	// alloc various tables
	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	cmap = AllocMem(cmapSize, MEMTYPE_ANY);
	columnPixels = (uint16*)AllocMem(VIS_HOR_STEPS * SCREEN_HEIGHT * sizeof(uint16), MEMTYPE_ANY);
	raySamples = (int*)AllocMem(VIS_VER_STEPS * VIS_HOR_STEPS * sizeof(int), MEMTYPE_ANY);
	viewNearPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_ANY);
	viewFarPoints = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_ANY);

	// load heightmap and colormap
	readBytesFromFileAndClose("data/hmap1.bin", 0, HMAP_SIZE, hmap);
	readBytesFromFileAndClose("data/cmap1.bin", 0, cmapSize, cmap);

	setVector3D(&viewPos, 3*HMAP_WIDTH/4, V_PLAYER_HEIGHT, HMAP_HEIGHT/6);
	setVector3D(&viewAngle, 0,128,0);

	initColumnCels();
	initEngineLUTs();
	createNonLinearTable();
	//createNonLinearTablePow();
	initRaySamplePoints();
	initShadedPals();
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

void effectVolumeScapeRun()
{
	updateFromInput();

	renderScape();
}
