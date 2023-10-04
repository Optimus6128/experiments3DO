#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"

#define FB_WIDTH 256
#define FB_HEIGHT 240
#define FB_SIZE (FB_WIDTH * FB_HEIGHT)


static Mesh *cubeMeshBack;
static Mesh *cubeMesh;
static Object3D *cubeBackObj;
static Object3D *cubeObj;

Texture *softFeedbackTex;
Texture *feedbackTex0;
Texture *draculTex;

static Sprite *bgndSpr;
unsigned char bgndBmp[FB_SIZE];

static bool flipPolygons = false;
static bool doFeedback = true;
static bool hwFeedback = true;
static bool translucency = false;

static int cubeOffsetX = 0;
static int cubeOffsetY = 0;

static Camera *camera;


static void genBackgroundTex()
{
	int x,y;
	unsigned char *dst = bgndBmp;

	for (y=0; y<FB_HEIGHT; ++y) {
		for (x=0; x<FB_WIDTH; ++x) {
			unsigned char r = 2;
			unsigned char g = 1;
			unsigned char b = 1;
			if (x==0 || x==FB_WIDTH-1 || y==0 || y==FB_HEIGHT-1) b=3;
			else if (x==1 || x==FB_WIDTH-2 || y==1 || y==FB_HEIGHT-2) b=2;
			*dst++ = (r << 5) | (g << 2) | b;
		}
	}
}

static void copyBufferToTexture()
{
	int x,y;
	uint32 *src = (uint32*)getBackBufferByIndex(feedbackTex0->bufferIndex);
	uint16 *tex = (uint16*)softFeedbackTex->bitmap;
	uint32 *dst0;
	uint32 *dst1;
	for (y=0; y<FB_HEIGHT/2; ++y) {
		dst0 = (uint32*)(tex + 2*y * FB_WIDTH);
		dst1 = (uint32*)(tex + (2*y+1) * FB_WIDTH);
		for (x=0; x<FB_WIDTH; x+=2) {
			uint32 s0 = *(src + x);
			uint32 s1 = *(src + x + 1);
			uint32 c0 = (s0 & ~65535) | (s1 >> 16);
			uint32 c1 = (s0 << 16) | (s1 & 65535);
			*dst0++ = c0;
			*dst1++ = c1;
		}
		src += SCREEN_WIDTH;
	}
}

static void switchFeedback(bool on)
{
	if (on) {
		feedbackTex0->type |= TEXTURE_TYPE_FEEDBACK;
	} else {
		feedbackTex0->type &= ~TEXTURE_TYPE_FEEDBACK;
		feedbackTex0->bitmap = softFeedbackTex->bitmap;
	}
	updateMeshCELs(cubeMesh);
}

static void inputScript()
{
	const int offsetSpeed = 4;

	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		hwFeedback = !hwFeedback;
		switchFeedback(hwFeedback);
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
		doFeedback = !doFeedback;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		flipPolygons = !flipPolygons;
		setMeshPolygonCPUbackfaceTest(cubeMesh, !flipPolygons);
		setMeshPolygonCPUbackfaceTest(cubeMeshBack, !flipPolygons);
		setMeshPolygonOrder(cubeMesh, flipPolygons, !flipPolygons);
		setMeshPolygonOrder(cubeMeshBack, flipPolygons, !flipPolygons);
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_LPAD)) {
		translucency = !translucency;
		setMeshTranslucency(cubeMesh, translucency, false);
		setMeshTranslucency(cubeMeshBack, translucency, false);
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_RPAD)) {
		setMeshPolygonCPUbackfaceTest(cubeMesh, false);
		setMeshPolygonCPUbackfaceTest(cubeMeshBack, false);
		setMeshPolygonOrder(cubeMesh, true, true);
		setMeshPolygonOrder(cubeMeshBack, true, true);
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		cubeOffsetX = 0;
		cubeOffsetY = 0;
	}
	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		cubeOffsetY += offsetSpeed;
	}
	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		cubeOffsetY -= offsetSpeed;
	}
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		cubeOffsetX -= offsetSpeed;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		cubeOffsetX += offsetSpeed;
	}
}

void effectFeedbackCubeInit()
{
	MeshgenParams params = DEFAULT_MESHGEN_PARAMS(256);

	feedbackTex0 = initFeedbackTexture(0, 0, FB_WIDTH, FB_HEIGHT, 0);
	softFeedbackTex = initTexture(FB_WIDTH, FB_HEIGHT, 16, TEXTURE_TYPE_DEFAULT, NULL, NULL, 0);
	draculTex = loadTexture("data/draculin.cel");

	cubeMesh = initGenMesh(MESH_CUBE, params, MESH_OPTION_CPU_POLYTEST, feedbackTex0);
	cubeMeshBack = initGenMesh(MESH_CUBE, params, MESH_OPTIONS_DEFAULT, draculTex);
	cubeObj = initObject3D(cubeMesh);
	cubeBackObj = initObject3D(cubeMeshBack);

	bgndSpr = newSprite(FB_WIDTH, FB_HEIGHT, 8, CEL_TYPE_UNCODED, NULL, bgndBmp);
	genBackgroundTex();

	switchFeedback(hwFeedback);

	camera = createCamera();
}

void effectFeedbackCubeRun()
{
	const int time = getFrameNum();

	inputScript();

	if (doFeedback) {
		setObject3Dpos(cubeBackObj, 0, 0, 512);
		setObject3Drot(cubeBackObj, time, time, time);

		switchRenderToBuffer(true);
		setScreenRegion(0, 0, FB_WIDTH, FB_HEIGHT);

			drawSprite(bgndSpr);
			renderObject3D(cubeBackObj, camera, NULL, 0);

		switchRenderToBuffer(false);
		setScreenRegion(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

		if (!hwFeedback)
			copyBufferToTexture();
	}


	setObject3Dpos(cubeObj, cubeOffsetX, cubeOffsetY, 512);
	setObject3Drot(cubeObj, 0, time, 0);

	renderObject3D(cubeObj, camera, NULL, 0);

	if (hwFeedback)
		drawText(320-32, 0, "HARD");
	else
		drawText(320-32, 0, "SOFT");

	if (!doFeedback)
		drawText(320-40, 8, "PAUSED");
}
