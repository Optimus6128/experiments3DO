#include "core.h"

#include "effect.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"

#include "sprite_engine.h"


static Mesh *pyramidMesh[3];
// COMMENT OUT PALETTE BUG
//static uint16 *pyramidPal;

static Texture *xorTexs, *xorTexs2;
static Sprite *sprTexture[3];

static int rotX=0, rotY=0, rotZ=0;
static int zoom=2048;

const int rotVel = 3;
const int zoomVel = 32;

static int pyramidMeshIndex = 0;
bool halfSizeBlur = false;

#define SCANLINES_SPR_WIDTH 320
#define SCANLINES_SPR_HEIGHT 240

static Sprite *scanlinesSpr;
static ubyte scanlinesBmp[(SCANLINES_SPR_WIDTH * SCANLINES_SPR_HEIGHT) / 8];
static ubyte *scanlinesPackedData = NULL;
static uint16 scanlinesPal[2];

static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		rotX += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		rotX -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		rotY += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		rotY -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		rotZ += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		rotZ -= rotVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		pyramidMeshIndex = (pyramidMeshIndex + 1) % 3;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		halfSizeBlur = !halfSizeBlur;
	}
}

static void renderText()
{
	char title[10];

	sprintf(title, "Method %d\0", pyramidMeshIndex+1);

	drawText(8,224, title);
}

static void renderTexture()
{
	drawSprite(sprTexture[pyramidMeshIndex]);
}

static void initScanlinesSprite()
{
	int x,y;
	ubyte *dst = scanlinesBmp;
	const int transparentColor = MakeRGB15(31,0,31);

	for (y=0; y<SCANLINES_SPR_HEIGHT; ++y) {
		int yp = y & 1;
		for (x=0; x<SCANLINES_SPR_WIDTH/8; ++x) {
			*dst++ = yp * 255;
		}
	}

	/*scanlinesPal[0] = MakeRGB15(0,0,0);
	scanlinesPal[1] = MakeRGB15(0,0,1);
	scanlinesSpr = newSprite(SCANLINES_SPR_WIDTH, SCANLINES_SPR_HEIGHT, 1, CREATECEL_CODED, scanlinesPal, scanlinesBmp);*/

	scanlinesPal[0] = MakeRGB15(0,0,0);
	scanlinesPal[1] = transparentColor;
	scanlinesSpr = newPackedSprite(SCANLINES_SPR_WIDTH, SCANLINES_SPR_HEIGHT, 1, CREATECEL_CODED, scanlinesPal, scanlinesBmp, NULL, transparentColor);
	scanlinesSpr->cel->ccb_Flags |= CCB_BGND;
}

void effectInit()
{
	int i;

	const int texWidth = 128;
	const int texHeight = texWidth;

	static uint16 pyramidPal[64];
	// COMMENT OUT PALETTE BUG
	//pyramidPal = (uint16*)AllocMem(2 * 32 * sizeof(uint16), MEMTYPE_ANY);

	//setPal(0,31, 48,64,192, 160,64,32, pyramidPal, 3);
	//setPal(32,63, 48,64,192, 160,64,32, pyramidPal, 3);

	setPal(0,31, 0,0,127, 0,0,255, pyramidPal, 3);
	setPal(32,63, 0,0,127, 0,0,255, pyramidPal, 3);

	xorTexs = initGenTexturesTriangleHack(texWidth,texHeight,8,pyramidPal,2,TEXGEN_XOR, NULL);
	xorTexs2 = initGenTexturesTriangleHack2(texWidth,texHeight,8,pyramidPal,2,TEXGEN_XOR, NULL);

	sprTexture[0] = newSprite(texWidth, texHeight, 8, CREATECEL_CODED, pyramidPal, xorTexs[0].bitmap);
	sprTexture[1] = newSprite(texWidth, texHeight, 8, CREATECEL_CODED, &pyramidPal[32], xorTexs[1].bitmap);
	sprTexture[2] = newSprite(texWidth, texHeight, 8, CREATECEL_CODED, pyramidPal, xorTexs2[1].bitmap);

	for (i=0; i<3; ++i) {
		sprTexture[i]->angle = 127;
		sprTexture[i]->zoom = 127;
		sprTexture[i]->posX = 280;
		sprTexture[i]->posY = 40;
		mapZoomRotateSprite(sprTexture[i]);
	}

	pyramidMesh[0] = initGenMesh(1024, xorTexs, MESH_OPTIONS_DEFAULT, MESH_PYRAMID1, NULL);
	pyramidMesh[1] = initGenMesh(1024, xorTexs, MESH_OPTIONS_DEFAULT, MESH_PYRAMID2, NULL);
	pyramidMesh[2] = initGenMesh(1024, xorTexs2, MESH_OPTIONS_DEFAULT, MESH_PYRAMID3, NULL);

	setMeshTransparency(pyramidMesh[1], true);

	initScanlinesSprite();
}

#define FPB (1<<15)
#define FPC (1<<13)
#define FPD (1<<12)
#define FPE (1<<10)

mat44f16 mat4a = {	0, FPB, FPB, 0, 
					0, FPB, FPB, 0,
					0, FPB, FPB, 0,
					0, FPB, FPB, 0};

mat44f16 mat4b = {	FPB, 0, FPB, 0, 
					0, FPB, 0, FPB,
					FPB, 0, FPB, 0,
					0, FPB, 0, FPB};

mat44f16 mat4c = {	FPB, FPC, FPD, FPE,
					FPC, FPB, FPC, FPD,
					FPD, FPC, FPB, FPC,
					FPE, FPD, FPC, FPB};


static void matrixBlurTest(int vecsNum)
{
	uint16 *vram = getVramBuffer();
	vec4f16 *src = (vec4f16*)vram;
	vec4f16 *dst = (vec4f16*)vram;

	MulManyVec4Mat44_F16(dst, src, mat4c, vecsNum);
}

void effectRun()
{
	Mesh *mesh = pyramidMesh[pyramidMeshIndex];

	inputScript();
	
	setMeshPosition(mesh, 0, 0, zoom);
	setMeshRotation(mesh, rotX, rotY, rotZ);

	transformGeometry(mesh);
	renderTransformedGeometry(mesh);

	drawSprite(scanlinesSpr);

	matrixBlurTest(9600 >> (int)halfSizeBlur);

	//renderTexture();
	renderText();
}
