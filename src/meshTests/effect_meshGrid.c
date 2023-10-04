#include "core.h"

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
static Texture *gridTex;
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

static bool gridTextureOn = true;
static bool halfInterp = false;
static bool plasmaInsteadOfWater = false;

Camera *camera;


static void updatePolyDataTextureShifts(int newWidth, int newHeight)
{
	const int texShrX = getShr(newWidth);
	const int texShrY = getShr(newHeight);
	const unsigned char texShifts = (texShrX << 4) | texShrY;

	PolyData *dstPoly = gridMesh->poly;
	int count = gridMesh->polysNum;

	do {
		dstPoly->texShifts = texShifts;
		++dstPoly;
	}while(--count > 0);
}

static void switchGridTexture()
{
	int x,y;

	unsigned char *cloudTexBmp = (unsigned char*)cloudTex->bitmap;
	int dx = cloudTex->width / WATER_SIZE;
	int dy = cloudTex->height / WATER_SIZE;

	CCB *cel = gridMesh->cel;

	for (y=0; y<WATER_SIZE; ++y) {
		for (x=0; x<WATER_SIZE; ++x) {
			if (gridTextureOn) {
				cel->ccb_SourcePtr = (CelData*)gridTex->bitmap;
				setCelStride(gridTex->width, cel);
				setCelWidth(gridTex->width, cel);
				setCelHeight(gridTex->height, cel);
			} else {
				cel->ccb_SourcePtr = (CelData*)(cloudTexBmp + ((WATER_SIZE-y-1) * dy) * cloudTex->width + x * dx);
				setCelStride(cloudTex->width, cel);
				setCelWidth(dx, cel);
				setCelHeight(dy, cel);
			}
			++cel;
		}
	}

	if (gridTextureOn) {
		updatePolyDataTextureShifts(gridTex->width, gridTex->height);
	} else {
		updatePolyDataTextureShifts(dx, dy);
	}
}

static void darkenAllCels()
{
	int count = gridMesh->polysNum;
	CCB *cel = gridMesh->cel;

	do {
		cel->ccb_PIXC = shadeTable[0];
		++cel;
	}while(--count > 0);
}

void effectMeshGridInit()
{
	MeshgenParams params = makeMeshgenGridParams(1024, WATER_SIZE);

	setPalGradient(0,31, 1,1,3, 27,29,31, waterPal);
	waterTex = initGenTexture(WATER_SIZE, WATER_SIZE, 8, waterPal, 1, TEXGEN_EMPTY, NULL);

	setPalGradient(0,31, 1,3,7, 15,23,31, floorPal);
	gridTex = initGenTexture(16,16, 8, floorPal, 1, TEXGEN_GRID, NULL);
	cloudTex = initGenTexture(128,128, 8, floorPal, 1, TEXGEN_CLOUDS, NULL);

	waterSpr = newSprite(WATER_SIZE, WATER_SIZE, 8, CEL_TYPE_CODED, waterPal, waterTex->bitmap);

	setSpritePositionZoom(waterSpr, 296, 24, 256);

	gridMesh = initGenMesh(MESH_GRID, params, MESH_OPTION_FAST_MAPCEL, gridTex);
	//setMeshTranslucency(gridMesh, true, false);
	gridObj = initObject3D(gridMesh);

	darkenAllCels();

	camera = createCamera();
}

static void waterRun()
{
	int x,y,c;
	int *b1=wb1,*b2=wb2,*bc;
	unsigned char *dst = (unsigned char*)waterTex->bitmap + WATER_SIZE + 1;

	//int t = getTicks();
	//int tx,ty;

    *(b1 + getRand(1, WATER_SIZE-2) + getRand(2, WATER_SIZE-3) * WATER_SIZE) = 4096;

 	//tx = WATER_SIZE/2 + (int)(sin((float)t/170.0f) * (WATER_SIZE/3-1));
 	//ty = WATER_SIZE/2 + (int)(sin((float)t/280.0f) * (WATER_SIZE/3-1));
    //*(b1 + tx + ty * WATER_SIZE) = 1024;

 	//tx = WATER_SIZE/2 + sin((float)t/350.0f) * (WATER_SIZE/3-1) - 1;
 	//ty = WATER_SIZE/2 + sin((float)t/260.0f) * (WATER_SIZE/3-1) - 1;
    //*(b1 + tx + ty * WATER_SIZE) = 512;

	bc=wb2; wb2=wb1; wb1=bc;

	for (y=1; y<WATER_SIZE-1; ++y) {
		for (x=1; x<WATER_SIZE-1; ++x) {
			//c = (( *(b1-1) + *(b1+1) + *(b1-WATER_SIZE) + *(b1+WATER_SIZE))>>1) - *b2;
			c = (( *(b1-1) + *(b1+1) + *(b1-WATER_SIZE) + *(b1+WATER_SIZE) + *(b1-1-WATER_SIZE) + *(b1-1+WATER_SIZE) + *(b1+1-WATER_SIZE) + *(b1+1+WATER_SIZE))>>2) - *b2;

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

static void plasmaRun()
{
	int x,y,c;
	unsigned char *dst = (unsigned char*)waterTex->bitmap + WATER_SIZE + 1;
	int t = getTicks()>>5;

	for (y=1; y<WATER_SIZE-1; ++y) {
		int yy = 2*(y-t) + 2*isin[(5*y-3*t)&255] - 24*t;
		for (x=1; x<WATER_SIZE-1; ++x) {

			c = (isin[(5*x + 3*y) & 255] >> 3) + (yy >> 4) + 16*t;

			c &= 511;
			if (c > 255) c = 511-c;
			*dst++ = c >> 3;
		}
		dst += 2;
	}
}

static void applyWaterBufferToGrid()
{
	int x,y;
	Vertex *dstVertex = gridMesh->vertex + WATER_SIZE + 2;
	unsigned char *src = (unsigned char*)waterTex->bitmap + WATER_SIZE + 1;
	CCB *cel = gridMesh->cel + WATER_SIZE + 1;

	for (y=1; y<WATER_SIZE-1; ++y) {
		for (x=1; x<WATER_SIZE-1; ++x) {
			const int c0 = *src;
			const int c1 = *(src+1);
			const int c2 = *(src+WATER_SIZE);
			const int c3 = *(src+WATER_SIZE+1);
			const int c = (c0 + c1 + c2 + c3) >> 2;

			if (halfInterp) {
				dstVertex->y = (dstVertex->y + c0) >> 1;
			} else {
				dstVertex->y = c0;
			}

			cel->ccb_PIXC = shadeTable[4 + (c >> 2)];
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
		gridTextureOn = !gridTextureOn;
		switchGridTexture();
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_SELECT)) {
		plasmaInsteadOfWater = !plasmaInsteadOfWater;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		halfInterp = !halfInterp;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}
}

void effectMeshGridRun()
{
	static int lele=0;

	inputScript();

	//setObject3Dpos(gridObj, getMousePosition().x, -getMousePosition().y, zoom);
	setObject3Dpos(gridObj, 0, -64, zoom);
	setObject3Drot(gridObj, rotX, rotY, rotZ);

	renderObject3D(gridObj, camera, NULL, 0);

	if (!halfInterp || ((lele++ & 1)==0)) {
		if (plasmaInsteadOfWater) {
			plasmaRun();
		} else {
			waterRun();
		}
	}

	applyWaterBufferToGrid();
	drawSprite(waterSpr);

	//renderText();
}
