#include "core.h"

#include "effect.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"
#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "procgen_texture.h"

#define DOTS_WIDTH 2
#define DOTS_HEIGHT 2
#define DOTS_DEPTH 16
#define DOTS_RANGE (DOTS_DEPTH / 2)
#define DOTS_NUM (DOTS_WIDTH * DOTS_HEIGHT * DOTS_DEPTH)

#define MESH_SCALE 4
#define TEX_WIDTH DOTS_DEPTH
#define TEX_HEIGHT (DOTS_DEPTH * 3/2)
#define TEX_DEPTH DOTS_DEPTH
#define TEX_SIZE_2D (TEX_WIDTH * TEX_HEIGHT)
#define TEX_SIZE (TEX_WIDTH * TEX_HEIGHT * TEX_DEPTH)

#define BLOB_WIDTH 10
#define BLOB_HEIGHT BLOB_WIDTH
#define BLOB_DEPTH BLOB_WIDTH
#define BLOB_SIZE (BLOB_WIDTH * BLOB_HEIGHT * BLOB_DEPTH)
#define BLOB_SIZE_2D (BLOB_WIDTH * BLOB_HEIGHT)

#define NUM_BLOBS 10

static int blobWaveX[NUM_BLOBS];
static int blobWaveY[NUM_BLOBS];
static int blobWaveZ[NUM_BLOBS];

static Vertex tv[DOTS_NUM];
static Vertex mv[DOTS_NUM];

static ubyte dotTexBmp[TEX_SIZE];
static uint16 dotTexPal[32];
static CCB *dotCel[DOTS_DEPTH];

static bool blendOn = true;
static bool mariaOn = false;

static int advRotate = 0;
static int advPlasma = 0;

#define NUM_SINES 4096

static int fsin1[NUM_SINES];
static int fsin2[NUM_SINES];
static int fsin3[NUM_SINES];
static int ftab[TEX_SIZE_2D];

static ubyte blob[BLOB_SIZE];

static int mouseTravelZ = 0;

static int posXtrans;
static int posYtrans;
static int posZtrans;
static int rotXtrans;
static int rotYtrans;
static int rotZtrans;


#define RANDTAB_SIZE 1024

static ubyte randbase[RANDTAB_SIZE];
static Mesh *torchMesh;
static Texture *torchTex;

static void updateBlending()
{
	int i;

	for (i=0; i<DOTS_DEPTH; ++i) {
		if (blendOn) {
			dotCel[i]->ccb_PIXC = 0x1F801F80;
		} else {
			dotCel[i]->ccb_PIXC = SOLID_CEL;
		}
	}
}

static void updateMaria()
{
	int i;

	for (i=0; i<DOTS_DEPTH; ++i) {
		if (mariaOn) {
			dotCel[i]->ccb_Flags |= CCB_MARIA;
		} else {
			dotCel[i]->ccb_Flags &= ~CCB_MARIA;
		}
	}
}

static void updatePlasma()
{
	//int x;
	int y, z, i = 0;

	const int t = advPlasma;

	for (z=0; z<TEX_DEPTH; ++z) {
		const int *fs2 = &fsin2[z+t];
		for (y=0; y<TEX_HEIGHT; ++y) {
			const int fs3 = fsin3[y+z+t+t];
			//for (x=0; x<TEX_WIDTH; ++x) {

				dotTexBmp[i] = (ftab[i] + fs2[0] + fs3) & 31;
				dotTexBmp[i+1] = (ftab[i+1] + fs2[1] + fs3) & 31;
				dotTexBmp[i+2] = (ftab[i+2] + fs2[2] + fs3) & 31;
				dotTexBmp[i+3] = (ftab[i+3] + fs2[3] + fs3) & 31;
				dotTexBmp[i+4] = (ftab[i+4] + fs2[4] + fs3) & 31;
				dotTexBmp[i+5] = (ftab[i+5] + fs2[5] + fs3) & 31;
				dotTexBmp[i+6] = (ftab[i+6] + fs2[6] + fs3) & 31;
				dotTexBmp[i+7] = (ftab[i+7] + fs2[7] + fs3) & 31;
				dotTexBmp[i+8] = (ftab[i+8] + fs2[8] + fs3) & 31;
				dotTexBmp[i+9] = (ftab[i+9] + fs2[9] + fs3) & 31;
				dotTexBmp[i+10] = (ftab[i+10] + fs2[10] + fs3) & 31;
				dotTexBmp[i+11] = (ftab[i+11] + fs2[11] + fs3) & 31;
				dotTexBmp[i+12] = (ftab[i+12] + fs2[12] + fs3) & 31;
				dotTexBmp[i+13] = (ftab[i+13] + fs2[13] + fs3) & 31;
				dotTexBmp[i+14] = (ftab[i+14] + fs2[14] + fs3) & 31;
				dotTexBmp[i+15] = (ftab[i+15] + fs2[15] + fs3) & 31;
				dotTexBmp[i+16] = (ftab[i+16] + fs2[16] + fs3) & 31;
				dotTexBmp[i+17] = (ftab[i+17] + fs2[17] + fs3) & 31;
				dotTexBmp[i+18] = (ftab[i+18] + fs2[18] + fs3) & 31;
				dotTexBmp[i+19] = (ftab[i+19] + fs2[19] + fs3) & 31;
				dotTexBmp[i+20] = (ftab[i+20] + fs2[20] + fs3) & 31;
				dotTexBmp[i+21] = (ftab[i+21] + fs2[21] + fs3) & 31;
				dotTexBmp[i+22] = (ftab[i+22] + fs2[22] + fs3) & 31;
				dotTexBmp[i+23] = (ftab[i+23] + fs2[23] + fs3) & 31;
				dotTexBmp[i+24] = (ftab[i+24] + fs2[24] + fs3) & 31;
				dotTexBmp[i+25] = (ftab[i+25] + fs2[25] + fs3) & 31;
				dotTexBmp[i+26] = (ftab[i+26] + fs2[26] + fs3) & 31;
				dotTexBmp[i+27] = (ftab[i+27] + fs2[27] + fs3) & 31;
				dotTexBmp[i+28] = (ftab[i+28] + fs2[28] + fs3) & 31;
				dotTexBmp[i+29] = (ftab[i+29] + fs2[29] + fs3) & 31;
				dotTexBmp[i+30] = (ftab[i+30] + fs2[30] + fs3) & 31;
				dotTexBmp[i+31] = (ftab[i+31] + fs2[31] + fs3) & 31;
				i += 32;
			//}
		}
	}
}

static void drawMetaBall(int px, int py, int pz)
{
	int x,y,z;
	ubyte *src = blob;
	ubyte *dst;

	const int ix = px - BLOB_WIDTH / 2;
	const int iy = py - BLOB_HEIGHT / 2;
	const int iz = pz - BLOB_DEPTH / 2;

	for (z=0; z<BLOB_DEPTH; ++z) {
		dst = &dotTexBmp[(iz + z) * TEX_SIZE_2D + iy * TEX_WIDTH + ix];
		for (y=0; y<BLOB_HEIGHT; ++y) {
			for (x=0; x<BLOB_WIDTH; ++x) {
				//*(dst + x) = (*(dst + x) + *(src + x)) >> 1;
				int c = *(src + x);
				if (c > 0) {
					c = *(dst + x) + c;
					if (c > 31) c = 31;
					*(dst + x) = c;
				}
			}
			src += BLOB_WIDTH;
			dst += TEX_WIDTH;
		}
	}
}

static void updateMetaballs()
{
	const int cx = TEX_WIDTH/2;
	const int cy = TEX_HEIGHT/2;
	const int cz = TEX_DEPTH/2;
	const int t = advPlasma;

	int i;

	memset(dotTexBmp, 0, TEX_SIZE);

	for (i=0; i<NUM_BLOBS; ++i) {
		int sx = (SinF16(t * blobWaveX[i]) * 9) >> 16;
		int sy = (SinF16(t * blobWaveY[i]) * 9) >> 16;
		int sz = (SinF16(t * blobWaveZ[i]) * 9) >> 16;
		drawMetaBall(cx+sx, cy+sy, cz+sz);
	}
}

static void updateFire()
{
	int x,y,z;

	const int randOffset = getRand(0, RANDTAB_SIZE - (TEX_WIDTH * TEX_DEPTH));
	ubyte *src = &randbase[randOffset];
	ubyte *dst;

	// Base
	for (y=0; y<2; ++y) {
		for (z=0; z<TEX_DEPTH; ++z) {
			const int zc = z - TEX_DEPTH / 2;
			dst = &dotTexBmp[z*TEX_SIZE_2D + TEX_SIZE_2D-(y+1)*TEX_WIDTH];
			for (x=0; x<TEX_WIDTH; ++x) {
				const int xc = x - TEX_WIDTH / 2;
				int d = (TEX_DEPTH * TEX_DEPTH) / 4 - (xc*xc + zc*zc);
				if (d < 0) d = 0;
				dst[x] = (d * (*src++)) >> 9;
			}
		}
	}

	for (z=1; z<TEX_DEPTH-1; ++z) {
		const int zz = (z & 1) + 1;
		for (y=1; y<TEX_HEIGHT-1; ++y) {
			const int yy = (y & 1) + 2;
			dst = &dotTexBmp[z*TEX_SIZE_2D + y*TEX_WIDTH];
			for (x=1; x<TEX_WIDTH-1; ++x) {
				int c = (dst[x-1+TEX_WIDTH] + dst[x+1+TEX_WIDTH] + yy*dst[x] + zz*dst[x+TEX_WIDTH] + dst[x-TEX_SIZE_2D+TEX_WIDTH] + dst[x+TEX_SIZE_2D+TEX_WIDTH]) >> 3;
				if (c > 31) c = 31;
				dst[x] = c;
			}
		}
	}
}

static void updateFx()
{
	//updatePlasma();
	//updateMetaballs();
	updateFire();
}

static Mesh *initTorchMesh()
{
	const int size = 256;

	int i;
	const int r1 = size/2;
	const int r2 = size/5;
	const int r3 = size/12;
	const int y1 = size/8;
	const int y2 = -size/8;
	const int y3 = -2*size;
	const int y4 = y3-size/4;

	Mesh* ms = initMesh(16, 12);

	ms->vrtx[0].x = -r1; ms->vrtx[0].y = y1; ms->vrtx[0].z = -r1;
	ms->vrtx[1].x = r1; ms->vrtx[1].y = y1; ms->vrtx[1].z = -r1;
	ms->vrtx[2].x = r1; ms->vrtx[2].y = y1; ms->vrtx[2].z = r1;
	ms->vrtx[3].x = -r1; ms->vrtx[3].y = y1; ms->vrtx[3].z = r1;

	ms->vrtx[4].x = -r2; ms->vrtx[4].y = y2; ms->vrtx[4].z = -r2;
	ms->vrtx[5].x = r2; ms->vrtx[5].y = y2; ms->vrtx[5].z = -r2;
	ms->vrtx[6].x = r2; ms->vrtx[6].y = y2; ms->vrtx[6].z = r2;
	ms->vrtx[7].x = -r2; ms->vrtx[7].y = y2; ms->vrtx[7].z = r2;

	ms->vrtx[8].x = -r2; ms->vrtx[8].y = y3; ms->vrtx[8].z = -r2;
	ms->vrtx[9].x = r2; ms->vrtx[9].y = y3; ms->vrtx[9].z = -r2;
	ms->vrtx[10].x = r2; ms->vrtx[10].y = y3; ms->vrtx[10].z = r2;
	ms->vrtx[11].x = -r2; ms->vrtx[11].y = y3; ms->vrtx[11].z = r2;

	ms->vrtx[12].x = -r3; ms->vrtx[12].y = y4; ms->vrtx[12].z = -r3;
	ms->vrtx[13].x = r3; ms->vrtx[13].y = y4; ms->vrtx[13].z = -r3;
	ms->vrtx[14].x = r3; ms->vrtx[14].y = y4; ms->vrtx[14].z = r3;
	ms->vrtx[15].x = -r3; ms->vrtx[15].y = y4; ms->vrtx[15].z = r3;

	for (i=0; i<3; ++i) {
		int *ind = &ms->index[i*16];
		const int viOff = i * 4;

		ind[0] = viOff + 0; ind[1] = viOff + 4; ind[2] = viOff + 5; ind[3] = viOff + 1;
		ind[4] = viOff + 1; ind[5] = viOff + 5; ind[6] = viOff + 6; ind[7] = viOff + 2;
		ind[8] = viOff + 2; ind[9] = viOff + 6; ind[10] = viOff + 7; ind[11] = viOff + 3;
		ind[12] = viOff + 3; ind[13] = viOff + 7; ind[14] = viOff + 4; ind[15] = viOff + 0;
	}
	
	torchTex = initGenTexture(16, 16, 8, NULL, 1, TEXGEN_NOISE, NULL);
	setPal(0, 31, 48,40,32, 96,80,40, torchTex->pal, 3);
	ms->tex = torchTex;

	for (i=0; i<ms->quadsNum; i++) {
		ms->quad[i].textureId = 0;
		ms->quad[i].palId = 0;
	}
	
	prepareCelList(ms);

	return ms;
}

void effectInit()
{
	int x, y, z, c, i = 0;
	const int palshr = 5;

	i = 0;
	for (z=-DOTS_RANGE; z<DOTS_RANGE; ++z) {
		for (y=-DOTS_RANGE; y<=DOTS_RANGE; y+=DOTS_DEPTH) {
			for (x=-DOTS_RANGE; x<=DOTS_RANGE; x+=DOTS_DEPTH) {
				mv[i].x = x << MESH_SCALE;
				mv[i].y = y << MESH_SCALE;
				mv[i].z = z << MESH_SCALE;
				++i;
			}
		}
	}

	for (i=0; i<DOTS_DEPTH; ++i) {
		dotCel[i] = CreateCel(TEX_WIDTH, TEX_HEIGHT, 8, CREATECEL_CODED, &dotTexBmp[i * TEX_SIZE_2D]);
		dotCel[i]->ccb_PLUTPtr = (uint16*)dotTexPal;
		dotCel[i]->ccb_Flags |= (CCB_ACW | CCB_ACCW | CCB_MARIA);
		if (i!=0) LinkCel(dotCel[i-1], dotCel[i]);
	}
	dotCel[DOTS_DEPTH-1]->ccb_Flags |= CCB_LAST;

	setPal(0, 7, 0,0,0, 0,0,0, dotTexPal, palshr);
	setPal(8, 14, 63,31,95, 128,96,48, dotTexPal, palshr);
	setPal(15, 27, 128,96,48, 255,128,32, dotTexPal, palshr);
	setPal(28, 31, 255,128,32, 255,255,128, dotTexPal, palshr);

	//setPal(0, 15, 0,0,0, 0,0,0, dotTexPal, palshr-2);
	//setPal(16, 31, 0,0,0, 255,255,255, dotTexPal, palshr-2);


	//setPal(0, 31, 0,0,0, 48,64,96, dotTexPal, 3);
	//setPal(0, 31, 0,0,0, 16,12,8, dotTexPal, 3);
	//setPal(0, 7, 0,0,0, 0,0,0, dotTexPal, 3);

	for (i=0; i<NUM_SINES; ++i) {
		fsin1[i] = SinF16(i << 16) >> 9;
		fsin2[i] = SinF16(i << 17) >> 10;
		fsin3[i] = SinF16(i << 18) >> 11;
	}

	i = 0;
	for (y=0; y<TEX_HEIGHT; ++y) {
		for (x=0; x<TEX_WIDTH; ++x) {
			ftab[i++] = fsin1[x+y];
		}
	}

	i = 0;
	for (z=0; z<BLOB_DEPTH; ++z) {
		float zc = (float)(z - BLOB_DEPTH / 2.0f);
		for (y=0; y<BLOB_HEIGHT; ++y) {
			float yc = (float)(y - BLOB_HEIGHT / 2.0f);
			for (x=0; x<BLOB_WIDTH; ++x) {
				float xc = (float)(x - BLOB_WIDTH / 2.0f);
				float r = sqrt(xc*xc + yc*yc + zc*zc);
				if (r==0.0f) r = 1.0f;

				c = 0;
				if (r <= BLOB_WIDTH/2.0f) c = (int)((1.0f - r / (BLOB_WIDTH/2.0f)) * 31.0f);

				blob[i++] = c;
			}
		}
	}

	for (i=0; i<NUM_BLOBS; ++i) {
		blobWaveX[i] = 2*65536 + getRand(0, 2*65536);
		blobWaveY[i] = 2*65536 + getRand(0, 2*65536);
		blobWaveZ[i] = 2*65536 + getRand(0, 2*65536);
	}

	for (i=0; i<RANDTAB_SIZE; ++i) {
		randbase[i] = getRand(128, 255);
	}

	torchMesh = initTorchMesh();

	updateFx();

	updateBlending();
	updateMaria();
}


static void drawCelInfo(CCB *cel)
{
	drawNumber(64,0, (int)cel->ccb_NextPtr);
	drawNumber(8,8, (int)cel->ccb_SourcePtr);
	drawNumber(8,16, (int)cel->ccb_PLUTPtr);

	drawNumber(8,32, cel->ccb_XPos);
	drawNumber(8,40, cel->ccb_YPos);
	drawNumber(8,48, cel->ccb_HDX);
	drawNumber(8,56, cel->ccb_HDY);
	drawNumber(8,64, cel->ccb_VDX);
	drawNumber(8,72, cel->ccb_VDY);
	drawNumber(8,80, cel->ccb_HDDX);
	drawNumber(8,88, cel->ccb_HDDY);

	drawNumber(8,104, cel->ccb_PIXC);
	drawNumber(8,112, cel->ccb_PRE0);
	drawNumber(8,120, cel->ccb_PRE1);

	drawNumber(8,136, cel->ccb_Width);
	drawNumber(8,144, cel->ccb_Height);
}

static void transformDotsHw(int rotX, int rotY, int rotZ)
{
    mat33f16 rotMat;

    createRotationMatrixValues(rotX, rotY, rotZ, (int*)rotMat);

    MulManyVec3Mat33_F16((vec3f16*)tv, (vec3f16*)mv, rotMat, DOTS_NUM);
}

static void renderMariaQuads(bool backToFront)
{
	int z;
	int pz;
	Vertex *tvPtr;
	Point2D q[4];

	// A lot of this code will be wiped out and replaced with mesh_procgen generating quad volume slices and then my 3D engine handling things instead
	// Here in this old example, I have basically duplicated code that could be done from the engine.
	// I am quickly changing things so that at least it can compile without errors after the big refactor, but it won't work yet.
	const int shrWidth = getShr(DOTS_DEPTH);
	const int shrHeight = getShr(DOTS_DEPTH);

	posXtrans = getMousePosition().x;
	posYtrans = getMousePosition().y;
	posZtrans = (8*DOTS_DEPTH + mouseTravelZ) << MESH_SCALE;

	tvPtr = tv;
	if (backToFront)
		tvPtr = &tv[DOTS_NUM-4];

	for (z=0; z<DOTS_DEPTH; ++z) {
		pz = tvPtr->z + posZtrans;
		q[0].x = ((((tvPtr->x + posXtrans) << PROJ_SHR) / pz)) + SCREEN_WIDTH / 2;
		q[0].y = ((((tvPtr->y + posYtrans) << PROJ_SHR) / pz)) + SCREEN_HEIGHT / 2;
		tvPtr++;

		pz = tvPtr->z + posZtrans;
		q[1].x = ((((tvPtr->x + posXtrans) << PROJ_SHR) / pz)) + SCREEN_WIDTH / 2;
		q[1].y = ((((tvPtr->y + posYtrans) << PROJ_SHR) / pz)) + SCREEN_HEIGHT / 2;
		tvPtr++;

		pz = tvPtr->z + posZtrans;
		q[3].x = ((((tvPtr->x + posXtrans) << PROJ_SHR) / pz)) + SCREEN_WIDTH / 2;
		q[3].y = ((((tvPtr->y + posYtrans) << PROJ_SHR) / pz)) + SCREEN_HEIGHT / 2;
		tvPtr++;

		pz = tvPtr->z + posZtrans;
		q[2].x = ((((tvPtr->x + posXtrans) << PROJ_SHR) / pz)) + SCREEN_WIDTH / 2;
		q[2].y = ((((tvPtr->y + posYtrans) << PROJ_SHR) / pz)) + SCREEN_HEIGHT / 2;
		tvPtr++;

		if (backToFront) {
			tvPtr -= 8;
		}

		{
			const int ptX0 = q[1].x - q[0].x;
			const int ptY0 = q[1].y - q[0].y;
			const int ptX1 = q[2].x - q[3].x;
			const int ptY1 = q[2].y - q[3].y;
			const int ptX2 = q[3].x - q[0].x;
			const int ptY2 = q[3].y - q[0].y;

			const int hdx0 = (ptX0 << 20) >> shrWidth;
			const int hdy0 = (ptY0 << 20) >> shrWidth;
			const int hdx1 = (ptX1 << 20) >> shrWidth;
			const int hdy1 = (ptY1 << 20) >> shrWidth;

			CCB *cel = dotCel[z];

			cel->ccb_XPos = q[0].x << 16;
			cel->ccb_YPos = q[0].y << 16;


			cel->ccb_HDX = hdx0;
			cel->ccb_HDY = hdy0;
			cel->ccb_VDX = (ptX2 << 16) >> shrHeight;
			cel->ccb_VDY = (ptY2 << 16) >> shrHeight;

			cel->ccb_HDDX = (hdx1 - hdx0) >> shrHeight;
			cel->ccb_HDDY = (hdy1 - hdy0) >> shrHeight;

			cel++;
		}
	}

	//drawCelInfo(dotCel[0]);

	drawCels(dotCel[0]);
}

static void renderTorch()
{
	torchMesh->posX = posXtrans;
	torchMesh->posY = -posYtrans - 256;
	torchMesh->posZ = posZtrans;

	torchMesh->rotX = rotXtrans;
	torchMesh->rotY = rotYtrans;
	torchMesh->rotZ = rotZtrans;

	renderMesh(torchMesh);
}

static void render()
{
	rotXtrans = 0;
	rotYtrans = advRotate;
	rotZtrans = 0;

    transformDotsHw(rotXtrans, rotYtrans, rotZtrans);

	renderMariaQuads(true);

	renderTorch();
}

static void updateFromInput()
{
	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		++advPlasma;
		updateFx();
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		--advRotate;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		++advRotate;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
		blendOn= !blendOn;
		updateBlending();
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		mariaOn = !mariaOn;
		updateMaria();
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		--mouseTravelZ;
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		++mouseTravelZ;
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
		advPlasma += 3;
		updateFx();
	}
}

void effectRun()
{
	updateFromInput();
	render();
}
