#include "core.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"
#include "file_utils.h"


//#define VERT_INTERP

#define FP_VIEWER 8
#define FP_SCAPE 10

#define FOV 48
#define VIS_NEAR 16
#define VIS_FAR 176

#define VIS_HOR_SKIP 2
#define VIS_VER_SKIP 2

#define VIS_VER_STEPS ((VIS_FAR - VIS_NEAR) / VIS_VER_SKIP)
#define VIS_HOR_STEPS (SCREEN_WIDTH / VIS_HOR_SKIP)

#define V_PLAYER_HEIGHT 176
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HEIGHT_SCALER (1 << V_HEIGHT_SCALER_SHIFT)

#define HMAP_WIDTH 512
#define HMAP_HEIGHT 512
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)



static int lintab[VIS_VER_STEPS];
static int heightScaleTab[VIS_VER_STEPS];

static Point2D *viewNearPosVec;
static Point2D *viewNearStepVec;

static unsigned char *hmap;

static CCB *columnCels;
static unsigned char *columnPixels;
static uint16 columnPal[32];

static Vector3D viewPos;
static Vector3D viewAngle;

static int horizon = SCREEN_HEIGHT / 2;

static bool walk = true;


static void initRaySamplePosAndStep()
{
	int i;
	Point2D pl, pr, dHor;
	
	const int halfFov = FOV / 2;
	const int yaw = viewAngle.y & 255;
	const int yawL = (yaw - halfFov) & 255;
	const int yawR = (yaw + halfFov) & 255;
	const int length = (1 << (FP_BASE + FP_SCAPE)) / isin[halfFov + 64];

	Point2D *viewPosVec = viewNearPosVec;
	Point2D *viewStepVec = viewNearStepVec;

	setPoint2D(&pl, isin[(yawL+64)&255]*length, isin[yawL]*length);
	setPoint2D(&pr, isin[(yawR+64)&255]*length, isin[yawR]*length);
	setPoint2D(&dHor, (pr.x - pl.x) / (VIS_HOR_STEPS - 1), (pr.y - pl.y) / (VIS_HOR_STEPS - 1));

	for (i=0; i<VIS_HOR_STEPS; ++i) {
		setPoint2D(viewStepVec++, (VIS_VER_SKIP * pl.x) >> FP_BASE, (VIS_VER_SKIP * pl.y) >> FP_BASE);
		setPoint2D(viewPosVec++, (VIS_NEAR * pl.x) >> FP_BASE, (VIS_NEAR * pl.y) >> FP_BASE);

		pl.x += dHor.x;
		pl.y += dHor.y;
	}
}

static void renderScape()
{
	int i,j;

	const int playerHeight = viewPos.y >> FP_VIEWER;
	const int viewerOffset = (viewPos.z >> FP_VIEWER) * HMAP_WIDTH + (viewPos.x >> FP_VIEWER);

	unsigned char *dstBase = columnPixels;
	for (j=0; j<VIS_HOR_STEPS; ++j) {
		int yMax = 0;

		#ifdef VERT_INTERP
			int8 cPrev = 0;
			bool shouldInterp = false;
		#endif

		int vx = viewNearPosVec[j].x;
		int vy = viewNearPosVec[j].y;
		const int dvx = viewNearStepVec[j].x;
		const int dvy = viewNearStepVec[j].y;

		unsigned char *dst = dstBase;
		for (i=0; i<VIS_VER_STEPS; ++i) {
			const int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
			const int mapOffset = (viewerOffset + sampleOffset) & (HMAP_SIZE - 1);
			const int hm = hmap[mapOffset];
			int h = (((-playerHeight + hm) * heightScaleTab[i]) >> (REC_FPSHR - V_HEIGHT_SCALER_SHIFT)) + horizon;
			if (h > SCREEN_HEIGHT-1) h = SCREEN_HEIGHT-1;

			if (yMax < h) {
				int hCount = h-yMax;

				#ifdef VERT_INTERP
					if (shouldInterp) {
						int cc = cPrev << REC_FPSHR;
						const int dc = (hm - cPrev) * recZ[hCount];

						do {
							*dst++ = (unsigned char)(cc >> (REC_FPSHR + 2));
							cc += dc;
						}while(--hCount > 0);
					} else {
						const unsigned char cv = hm >> 2;
						do {
							*dst++ = cv;
						}while(--hCount > 0);
					}
				#else
					const unsigned char cv = hm >> 2;
					do {
						*dst++ = cv;
					}while(--hCount > 0);
				#endif

				yMax = h;
				if (yMax == SCREEN_HEIGHT - 1) break;

				#ifdef VERT_INTERP
					cPrev = hm;
					shouldInterp = true;
				#endif
			}
			#ifdef VERT_INTERP
			else {
				shouldInterp = false;
			}
			#endif

			vx += dvx;
			vy += dvy;
		}

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

static void createLinearTable()
{
	int i;
	float zone = 0;
	float dz = 1.0f / (VIS_VER_STEPS - 1);
	for (i = 0; i < VIS_VER_STEPS; ++i) {
		float inter = zone; //pow(zone, 1.0f);
		lintab[i] = (int)(inter * (1 << FP_SCAPE));
		heightScaleTab[i] = recZ[VIS_NEAR + ((lintab[i] * VIS_FAR) >> FP_SCAPE)];
		zone += dz;
	}
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

static void setPlayerPos(int px, int py, int pz)
{
	setVector3D(&viewPos, px << FP_VIEWER, py << FP_VIEWER, pz << FP_VIEWER);
}

void effectVolumeScapeGradientInit()
{
	// alloc various tables
	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	columnPixels = (unsigned char*)AllocMem(VIS_HOR_STEPS * SCREEN_HEIGHT, MEMTYPE_ANY);

	viewNearPosVec = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_TRACKSIZE);
	viewNearStepVec = (Point2D*)AllocMem(VIS_HOR_STEPS * sizeof(Point2D), MEMTYPE_TRACKSIZE);

	// load heightmap and colormap
	readBytesFromFileAndClose("data/hmap1.bin", 0, HMAP_SIZE, hmap);

	setPlayerPos(3*HMAP_WIDTH/4, V_PLAYER_HEIGHT, HMAP_HEIGHT/6);
	setVector3D(&viewAngle, 0,128,0);

	initColumnCels();
	initEngineLUTs();

	createLinearTable();
}

static void updateFromInput()
{
	const int speedX = 3 << FP_VIEWER;
	const int speedY = 2 << FP_VIEWER;
	const int speedZ = 3 << FP_VIEWER;

	const int speedA = 4;
	const int tiltSpeed = 8;

	const int velX = (speedX * isin[(viewAngle.y + 64) & 255]) >> FP_BASE;
	const int velZ = (speedZ * isin[viewAngle.y]) >> FP_BASE;

	bool tiltView = false;
	if (isJoyButtonPressed(JOY_BUTTON_C)) {
		tiltView = true;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		if (tiltView) {
			if (horizon < SCREEN_HEIGHT - 1) horizon += tiltSpeed;
		} else {
			viewPos.x += velX;
			viewPos.z += velZ;
		}
	}
	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		if (tiltView) {
			if (horizon > 0) horizon -= tiltSpeed;
		} else {
			viewPos.x -= velX;
			viewPos.z -= velZ;
		}
	}
	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		viewPos.x += velZ;
		viewPos.z -= velX;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		viewPos.x -= velZ;
		viewPos.z += velX;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		viewAngle.y = (viewAngle.y - speedA) & 255;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		viewAngle.y = (viewAngle.y + speedA) & 255;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		viewPos.y += speedY;
	}
	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		viewPos.y -= speedY;
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
		viewPos.y = (hmap[((viewPos.z >> FP_VIEWER) * HMAP_WIDTH + (viewPos.x >> FP_VIEWER)) & (HMAP_SIZE - 1)] + V_PLAYER_HEIGHT/4) << FP_VIEWER;
	}
}

void effectVolumeScapeGradientRun()
{
	updateFromInput();

	initRaySamplePosAndStep();
	renderScape();
}
