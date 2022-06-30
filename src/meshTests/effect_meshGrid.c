#include "core.h"

#include "effect_meshGrid.h"

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


static Mesh *gridMesh;
static Object3D *gridObj;

static Texture *waterTex;
static Texture *cloudTex;
static Sprite *waterSpr;
static uint16 waterPal[32];
static uint16 floorPal[32];

#define WATER_SIZE 32

int waterBuff1[WATER_SIZE * WATER_SIZE];
int waterBuff2[WATER_SIZE * WATER_SIZE];

int *wb1 = waterBuff1 + WATER_SIZE + 1, *wb2 = waterBuff2 + WATER_SIZE + 1;

static int rotX=16, rotY=0, rotZ=0;
static int zoom=1024;

static const int rotVel = 2;
static const int zoomVel = 8;


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

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}
}

static void hackQuarterTex()
{
	int x,y;
	const int texWidth = cloudTex->width;
	const int texHeight = cloudTex->height;
	const int texWidthHalf = texWidth/2;
	const int texHeightHalf = texHeight/2;

	uint8 *bmp = cloudTex->bitmap;
	for (y=0; y<texHeightHalf; ++y) {
		for (x=0; x<texWidthHalf; ++x) {
			uint8 c = *(bmp + y*texWidth + x);
			*(bmp + y*texWidth + texWidth-1-x) = c;
			*(bmp + (texHeight-1-y)*texWidth + x) = c;
			*(bmp + (texHeight-1-y)*texWidth + texWidth-1-x) = c;
		}
	}
}

void effectMeshGridInit()
{
	MeshgenParams params = makeMeshgenGridParams(1024, WATER_SIZE);

	setPalGradient(0,31, 1,1,3, 27,29,31, waterPal);
	waterTex = initGenTexture(WATER_SIZE, WATER_SIZE, 8, waterPal, 1, TEXGEN_EMPTY, false, NULL);

	setPalGradient(0,31, 1,5,3, 23,27,31, floorPal);
	//cloudTex = initGenTexture(16,16, 8, waterPal, 1, TEXGEN_GRID, false, NULL);
	cloudTex = initGenTexture(32,32, 8, floorPal, 1, TEXGEN_CLOUDS, false, NULL);
	hackQuarterTex();

	waterSpr = newSprite(WATER_SIZE, WATER_SIZE, 8, CEL_TYPE_CODED, waterPal, waterTex->bitmap);

	setSpritePositionZoom(waterSpr, 296, 24, 256);

	gridMesh = initGenMesh(MESH_GRID, params, MESH_OPTIONS_DEFAULT, cloudTex);
	//setMeshTranslucency(gridMesh, true);
	gridObj = initObject3D(gridMesh);
}

static void waterRun()
{
	int x,y,c,tx,ty;
	int *b1=wb1,*b2=wb2,*bc;
	uint8 *dst = (uint8*)waterTex->bitmap + WATER_SIZE + 1;
	int t = getTicks();


    //*(b1 + getRand(1, WATER_SIZE-2) + getRand(2, WATER_SIZE-3) * WATER_SIZE) = 4096;

 	tx = WATER_SIZE/2 + (int)(sin((float)t/170.0f) * (WATER_SIZE/3-1));
 	ty = WATER_SIZE/2 + (int)(sin((float)t/280.0f) * (WATER_SIZE/3-1));
    *(b1 + tx + ty * WATER_SIZE) = 1024;

 	tx = WATER_SIZE/2 + sin((float)t/350.0f) * (WATER_SIZE/3-1) - 1;
 	ty = WATER_SIZE/2 + sin((float)t/260.0f) * (WATER_SIZE/3-1) - 1;
    *(b1 + tx + ty * WATER_SIZE) = 512;

	bc=wb2; wb2=wb1; wb1=bc;

	for (y=1; y<WATER_SIZE-1; ++y) {
		for (x=1; x<WATER_SIZE-1; ++x) {
			c = (( *(b1-1) + *(b1+1) + *(b1-WATER_SIZE) + *(b1+WATER_SIZE))>>1) - *b2;
			// c = (( *(b1-1) + *(b1+1) + *(b1-WATER_SIZE) + *(b1+WATER_SIZE) + *(b1-1-WATER_SIZE) + *(b1-1+WATER_SIZE) + *(b1+1-WATER_SIZE) + *(b1+1+WATER_SIZE))>>2) - *b2;

			if (c < 0) c = -c;
			if (c>255) c=255;
			*b2++=c;
			b1++;

			*dst++ = c >> 3;
		}
		b1+=2;
		b2+=2;
		dst += 2;
	}
}

static void applyWaterBufferToGrid()
{
	int x,y;
	Vertex *dstVertex = gridMesh->vertex + WATER_SIZE + 2;
	uint8 *src = (uint8*)waterTex->bitmap + WATER_SIZE + 1;
	CCB *cel = gridMesh->cel + WATER_SIZE + 1;

	for (y=1; y<WATER_SIZE-1; ++y) {
		for (x=1; x<WATER_SIZE-1; ++x) {
			const int c0 = *src;
			const int c1 = *(src+1);
			const int c2 = *(src+WATER_SIZE);
			const int c3 = *(src+WATER_SIZE+1);
			const int c = (c0 + c1 + c2 + c3) >> 2;

			dstVertex->y = c0;
			cel->ccb_PIXC = shadeTable[c >> 1];
			++src;
			++cel;
			++dstVertex;
		}
		dstVertex+=3;
		cel+=2;
		src+=2;
	}
}


/*static void renderText()
{
	drawNumber(8,200, zoom);
	drawNumber(8,208, rotX);
	drawNumber(8,216, rotY);
	drawNumber(8,224, rotZ);
}*/

void effectMeshGridRun()
{
	inputScript();

	//setObject3Dpos(gridObj, getMousePosition().x, -getMousePosition().y, zoom);
	setObject3Dpos(gridObj, 0, -64, zoom);
	setObject3Drot(gridObj, rotX, rotY, rotZ);

	renderObject3D(gridObj);

	waterRun();
	applyWaterBufferToGrid();
	drawSprite(waterSpr);

	//renderText();
}
