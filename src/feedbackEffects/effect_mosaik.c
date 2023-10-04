#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"

#include "sprite_engine.h"
#include "celutils.h"

static Mesh *draculMesh;
static Object3D *draculObj;
static Texture *draculTex;

static Sprite *feedbackSpr1;
static Sprite *feedbackSpr2;

CCB *starryBack;
uint32 *origBackSourcePtr;

static Camera *camera;


void effectMosaikInit()
{
	starryBack = LoadCel("data/starry480.cel", MEMTYPE_ANY);
	starryBack->ccb_PRE0 = (starryBack->ccb_PRE0 & ~PRE0_VCNT_MASK) | (239 << PRE0_VCNT_SHIFT);	// restrict to 240p, later offset the src point to scroll vertically
	origBackSourcePtr = (uint32*)starryBack->ccb_SourcePtr;

	draculTex = loadTexture("data/draculin64.cel");
	draculMesh = initGenMesh(MESH_CUBE, DEFAULT_MESHGEN_PARAMS(256), MESH_OPTIONS_DEFAULT, draculTex);
	draculObj = initObject3D(draculMesh);

	feedbackSpr1 = newFeedbackSprite(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	feedbackSpr2 = newFeedbackSprite(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1);

	camera = createCamera();
}

static void renderDraculCube(int t)
{
	setObject3Dpos(draculObj, 0, 0, 512);
	setObject3Drot(draculObj, t, t<<1, t>>1);
	renderObject3D(draculObj, camera, NULL, 0);
}

static void renderScrollBackground()
{
	static int scrollY = 0;
	static int scrollYdir = 1;

	scrollY += scrollYdir;
	if (scrollY <= 0 || scrollY >= 240) {
		scrollYdir = -scrollYdir;
	}
	starryBack->ccb_SourcePtr = (CelData*)(origBackSourcePtr + scrollY * 80);

	drawCels(starryBack);
}

void effectMosaikRun()
{
	const int time = getFrameNum();


	int z = (int)(cos(time*0.01f) * 252.0f);

	z = 256 - z;
	if (z < 256) {
		setRenderBuffer(0);
		switchRenderToBuffer(true);
	}

	renderScrollBackground();
	renderDraculCube(time);

	if (z < 256) {
		const int shrinkX = ((SCREEN_WIDTH*z) >> 8) & ~1;
		const int shrinkY = ((SCREEN_HEIGHT*z) >> 8) & ~1;

		setRenderBuffer(1);
		switchRenderToBuffer(true);
		mapZoomSpriteToQuad(feedbackSpr1, 0,0, shrinkX,shrinkY);
		drawSprite(feedbackSpr1);

		switchRenderToBuffer(false);
		mapFeedbackSpriteToNewFramebufferArea(0,0, shrinkX, shrinkY, 1, feedbackSpr2);
		mapZoomSpriteToQuad(feedbackSpr2, 0,0, SCREEN_WIDTH,SCREEN_HEIGHT);
		drawSprite(feedbackSpr2);
	}
}
