#include "core.h"

#include "effect.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#define DRACUL_SIZE 128
#define DRACUL_SHR 7


static mesh *cubeMeshBack;
static mesh *cubeMesh;

static Sprite *bgndSpr;
ubyte bgndBmp[FB_SIZE];

static bool doFeedback = true;
static bool hwFeedback = false;

static uint16 *cubeTex;


static void convertDracul()
{
    int x,y,i;

    i = 0;
    for (y=0; y<DRACUL_SIZE; ++y) {
        for (x=0; x<DRACUL_SIZE; ++x) {
            ushort c = dracul[i];
            int r = c & 31;
            int bg = ((c >> 5) & 2047) >> 1;
            dracul[i++] = (bg << 5) | r;
        }
    }
}

static void genBackgroundTex()
{
    int x,y;
    ubyte *dst = bgndBmp;

    for (y=0; y<FB_HEIGHT; ++y) {
        for (x=0; x<FB_WIDTH; ++x) {
            ubyte r = 2;
            ubyte g = 1;
            ubyte b = 1;
            if (x==0 || x==FB_WIDTH-1 || y==0 || y==FB_HEIGHT-1) b=3;
            else if (x==1 || x==FB_WIDTH-2 || y==1 || y==FB_HEIGHT-2) b=2;
            *dst++ = (r << 5) | (g << 2) | b;
        }
    }
}

static void copyBufferToTexture()
{
    int x,y;
    uint32 *src = (uint32*)getBackBuffer();
    uint16 *tex = (uint16*)(cubeMesh->quad[0].cel->ccb_SourcePtr);
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
	int i;
	const int feedbackFlag = 1 << 11;

	for (i=0; i<cubeMesh->quadsNum; i++) {
        CCB *cel = cubeMesh->quad[i].cel;
        int woffset;
        int vcnt;

        if (on) {
            cel->ccb_PRE1 |= feedbackFlag;
            cel->ccb_SourcePtr = (void*)getBackBuffer();
            woffset = SCREEN_WIDTH - 2;
            vcnt = (FB_HEIGHT / 2) - 1;
        } else {
            cel->ccb_PRE1 &= ~feedbackFlag;
            cel->ccb_SourcePtr = (void*)cubeTex;
            woffset = FB_WIDTH / 2 - 2;
            vcnt = FB_HEIGHT - 1;
        }
        cel->ccb_PRE0 = (cel->ccb_PRE0 & ~(((1<<10) - 1)<<6)) | (vcnt << 6);
        cel->ccb_PRE1 = (cel->ccb_PRE1 & 65535) | (woffset << 16);
	}
}

static void inputScript()
{
    if (isButtonPressedOnce(BUTTON_B)) {
        doFeedback = !doFeedback;
    }

    if (isButtonPressedOnce(BUTTON_C)) {
        hwFeedback = !hwFeedback;
        switchFeedback(hwFeedback);
    }
}

void effectInit()
{
    convertDracul();
	
	initTexture(FB_WIDTH, FB_HEIGHT, TEXTURE_NOISE, 16);
	initTexture(128, 128, TEXTURE_DRACUL, 16);
	
    cubeMesh = initMesh(MESH_CUBE, 256, 1, TEXTURE_NOISE);
    cubeMeshBack = initMesh(MESH_CUBE, 256, 1, TEXTURE_DRACUL);

    bgndSpr = newSprite(FB_WIDTH, FB_HEIGHT, 8, CREATECEL_UNCODED, NULL, bgndBmp);
    genBackgroundTex();

    cubeTex = (uint16*)(cubeMesh->quad[0].cel->ccb_SourcePtr);
}

void effectRun()
{
	const int time = getFrameNum();

	inputScript();

	if (doFeedback) {
        setMeshPosition(cubeMeshBack, 0, 0, 512);
        setMeshRotation(cubeMeshBack, time, time, time);

        switchBuffer(true);
		setScreenDimensions(FB_WIDTH, FB_HEIGHT);

            drawSprite(bgndSpr);
            rotateTranslateProjectVertices(cubeMeshBack);
            renderTransformedGeometry(cubeMeshBack);

        switchBuffer(false);
		setScreenDimensions(SCREEN_WIDTH, SCREEN_HEIGHT);

        if (!hwFeedback)
            copyBufferToTexture();
	}


    setMeshPosition(cubeMesh, 0, 0, 512);
	setMeshRotation(cubeMesh, 0, time, 0);

    rotateTranslateProjectVertices(cubeMesh);
    renderTransformedGeometry(cubeMesh);

    if (hwFeedback)
        drawText(320-36, 4, "HARD");
    else
        drawText(320-36, 4, "SOFT");
}
