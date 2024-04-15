#include "types.h"
#include "tools.h"
#include "cel_helpers.h"

#include "core.h"
#include "timerutils.h"
#include "system_graphics.h"
#include "input.h"

static unsigned char bitfonts[] = {0,0,0,0,0,0,0,0,
4,12,8,24,16,0,32,0,
10,18,20,0,0,0,0,0,
0,20,126,40,252,80,0,0,
6,25,124,32,248,34,28,0,
4,12,72,24,18,48,32,0,
14,18,20,8,21,34,29,0,
32,32,64,0,0,0,0,0,
16,32,96,64,64,64,32,0,
4,2,2,2,6,4,8,0,
8,42,28,127,28,42,8,0,
0,4,12,62,24,16,0,0,
0,0,0,0,0,0,32,64,
0,0,0,60,0,0,0,0,
0,0,0,0,0,0,32,0,
4,12,8,24,16,48,32,0,

14,17,35,77,113,66,60,0,
12,28,12,8,24,16,16,0,
30,50,4,24,48,96,124,0,
28,50,6,4,2,98,60,0,
2,18,36,100,126,8,8,0,
15,16,24,4,2,50,28,0,
14,17,32,76,66,98,60,0,
126,6,12,24,16,48,32,0,
56,36,24,100,66,98,60,0,
14,17,17,9,2,34,28,0,

0,0,16,0,0,16,0,0,
0,0,16,0,16,32,0,0,
0,0,0,0,0,0,0,0,
0,0,30,0,60,0,0,0,
0,0,0,0,0,0,0,0,
28,50,6,12,8,0,16,0,
0,0,0,0,0,0,0,0,

14,27,51,63,99,65,65,0,
28,18,57,38,65,65,62,0,
14,25,32,96,64,98,60,0,
60,34,33,97,65,66,124,0,
30,32,32,120,64,64,60,0,
31,48,32,60,96,64,64,0,
14,25,32,96,68,98,60,0,
17,17,50,46,100,68,68,0,
8,8,24,16,48,32,32,0,
2,2,2,6,68,68,56,0,
16,17,54,60,120,76,66,0,
16,48,32,96,64,64,60,0,
10,21,49,33,99,66,66,0,
17,41,37,101,67,66,66,0,
28,50,33,97,67,66,60,0,
28,50,34,36,120,64,64,0,
28,50,33,97,77,66,61,0,
28,50,34,36,124,70,66,0,
14,25,16,12,2,70,60,0,
126,24,16,16,48,32,32,0,
17,49,35,98,70,68,56,0,
66,102,36,44,40,56,48,0,
33,97,67,66,86,84,40,0,
67,36,24,28,36,66,66,0,
34,18,22,12,12,8,24,0,
31,2,4,4,8,24,62,0};


#define MAX_DEBUG_NUMS 256

static int debugNums[MAX_DEBUG_NUMS];
static int debugIndex = 0;


static CCB *textCel[MAX_STRING_LENGTH];

static char sbuffer[MAX_STRING_LENGTH+1];
static unsigned char fontsBmp[NUM_TINY_FONTS * TINY_FONT_SIZE];
static uint16 fontsPal[TINY_FONTS_PAL_SIZE];
static unsigned char fontsMap[TINY_FONTS_MAP_SIZE];

static bool fontsAreReady = false;

static CCB *selectionBarCel = NULL;

static Item timerIOreq = -1;


static void initTimer()
{
	if (timerIOreq < 0) {
		timerIOreq = GetTimerIOReq();
	}
}

static void initTinyFonts()
{
	int i = 0;
	int n, x, y;

	if (fontsAreReady) return;

	for (n=0; n<TINY_FONTS_PAL_SIZE; ++n) {
		fontsPal[n] = MakeRGB15(n, n, n);
	}

	for (n=0; n<NUM_TINY_FONTS; n++) {
		for (y=0; y<TINY_FONT_HEIGHT; y++) {
			int c = bitfonts[i++];
			for (x=0; x<TINY_FONT_WIDTH; x++) {
				fontsBmp[n * TINY_FONT_SIZE + x + y * TINY_FONT_WIDTH] = ((c >>  (7 - x)) & 1) * (TINY_FONTS_PAL_SIZE - 1);
			}
		}
	}

	for (i=0; i<TINY_FONTS_MAP_SIZE; ++i) {
		unsigned char c = i;

		if (c>31 && c<92)
			c-=32;
		else
			if (c>96 && c<123) c-=64;
		else
			c = 255;

		fontsMap[i] = c;
	}

	for (i=0; i<MAX_STRING_LENGTH; ++i) {
		textCel[i] = createCel(TINY_FONT_WIDTH, TINY_FONT_HEIGHT, 8, CEL_TYPE_CODED);
		setupCelData(fontsPal, fontsBmp, textCel[i]);

		textCel[i]->ccb_HDX = 1 << 20;
		textCel[i]->ccb_HDY = 0 << 20;
		textCel[i]->ccb_VDX = 0 << 16;
		textCel[i]->ccb_VDY = 1 << 16;

		textCel[i]->ccb_Flags |= (CCB_ACSC | CCB_ALSC);

		if (i > 0) linkCel(textCel[i-1], textCel[i]);
	}

	fontsAreReady = true;
}

void setTextColor(uint16 color)
{
	fontsPal[31] = color;
}

void initTools()
{
	initTimer();
	initTinyFonts();
}

void drawZoomedText(int xtp, int ytp, char *text, int zoom)
{
	int i = 0;
	unsigned char c;

	if (!fontsAreReady) return;

	do {
		c = fontsMap[*text++];

		textCel[i]->ccb_XPos = xtp  << 16;
		textCel[i]->ccb_YPos = ytp  << 16;

		textCel[i]->ccb_HDX = (zoom << 20) >> TEXT_ZOOM_SHR;
		textCel[i]->ccb_VDY = (zoom << 16) >> TEXT_ZOOM_SHR;

		textCel[i]->ccb_SourcePtr = (CelData*)&fontsBmp[c * TINY_FONT_SIZE];

		xtp+= ((zoom * TINY_FONT_WIDTH) >> TEXT_ZOOM_SHR);
	} while(c!=255 && ++i < MAX_STRING_LENGTH);

	--i;
	textCel[i]->ccb_Flags |= CCB_LAST;
	drawCels(textCel[0]);
	textCel[i]->ccb_Flags &= ~CCB_LAST;
}

void drawTextX2(int xtp, int ytp, char *text)
{
	drawZoomedText(xtp, ytp, text, 2 << TEXT_ZOOM_SHR);
}

void drawText(int xtp, int ytp, char *text)
{
	drawZoomedText(xtp, ytp, text, 1 << TEXT_ZOOM_SHR);
}

void drawNumber(int xtp, int ytp, int num)
{
	sprintf(sbuffer, "%d", num);	// investigate and see if we can replace sprintf for numbers (I have encountered possible performance drawback for many (80) values per frame)
	drawText(xtp, ytp, sbuffer);
}


int getTicks()
{
	//return GetTime(timerIOreq);
	return GetMSecTime(timerIOreq);
}

void displayFPS()
{
	static int fps = 0, prevFrameNum = 0, prevTicks = 0;

	if (getTicks() - prevTicks >= 1000)
	{
		const int frameNum = getFrameNum();

		prevTicks = getTicks();
		fps = frameNum - prevFrameNum;
		prevFrameNum = frameNum;
	}

	drawNumber(16, 16, fps);
}

void displayMem()
{
	MemInfo memInfoAny;
	MemInfo memInfoDRAM;
	MemInfo memInfoVRAM;

	const int xp = 0;
	int yp = SCREEN_HEIGHT - (3 * 2 * 8) - 8;

	AvailMem(&memInfoAny, MEMTYPE_ANY);
	AvailMem(&memInfoDRAM, MEMTYPE_DRAM);
	AvailMem(&memInfoVRAM, MEMTYPE_VRAM);

	drawText(xp, yp, " ANY FREE:");
	drawNumber(xp + 11*8, yp, memInfoAny.minfo_SysFree); yp += 8;
	drawText(xp, yp, " ANY WIDE:");
	drawNumber(xp + 11*8, yp, memInfoAny.minfo_SysLargest); yp += 8;

	drawText(xp, yp, "DRAM FREE:");
	drawNumber(xp + 11*8, yp, memInfoDRAM.minfo_SysFree); yp += 8;
	drawText(xp, yp, "DRAM WIDE:");
	drawNumber(xp + 11*8, yp, memInfoDRAM.minfo_SysLargest); yp += 8;

	drawText(xp, yp, "VRAM FREE:");
	drawNumber(xp + 11*8, yp, memInfoVRAM.minfo_SysFree); yp += 8;
	drawText(xp, yp, "VRAM WIDE:");
	drawNumber(xp + 11*8, yp, memInfoVRAM.minfo_SysLargest);
}

void displayBuffers()
{
	const int xp = 0;
	const int yp = 8;

	drawText(xp, yp, "VRAM: ");
	drawNumber(xp + 6*8, yp, getNumVramBuffers());
	drawText(xp, yp+8, "OFFS: ");
	drawNumber(xp + 6*8, yp+8, getNumOffscreenBuffers());
}

static void renderEffectSelectorBar(int px, int py, int length)
{
	if (!selectionBarCel) {
		selectionBarCel = CreateBackdropCel(SCREEN_WIDTH, 8, 0x7FFF, 100);
		selectionBarCel->ccb_Flags |= CCB_PXOR;
		selectionBarCel->ccb_PIXC = 0x1F801F80;
	}

	selectionBarCel->ccb_XPos = px << 16;
	selectionBarCel->ccb_YPos = py << 16;
	selectionBarCel->ccb_HDX = length << 20;

	drawCels(selectionBarCel);
}

int runEffectSelector(char **str, int size)
{
	int i;
	int selection = 0;
	const int px = 24;
	const int py = 16;

	coreInit(NULL, CORE_VRAM_SINGLEBUFFER | CORE_NO_CLEAR_FRAME);

	// Display menu once
	for (i=0; i<size; ++i) {
		drawText(px, py+i*TINY_FONT_WIDTH, str[i]);
	}
	renderEffectSelectorBar(px, py, strlen(str[selection])*TINY_FONT_WIDTH);

	do {
		int moveOffset = 0;

		updateInput();

		if (isJoyButtonPressedOnce(JOY_BUTTON_UP)) {
			if (selection > 0) {
				moveOffset = -1;
			}
		}
		if (isJoyButtonPressedOnce(JOY_BUTTON_DOWN)) {
			if (selection < size-1) {
				moveOffset = 1;
			}
		}
		if (isJoyButtonPressedOnce(JOY_BUTTON_A) || isJoyButtonPressedOnce(JOY_BUTTON_START)) {
			DeleteCel(selectionBarCel);
			deInitGraphics();
			return selection;
		}

		if (moveOffset!=0) {
			renderEffectSelectorBar(px, py + selection*TINY_FONT_WIDTH, strlen(str[selection])*TINY_FONT_WIDTH);
			selection += moveOffset;
			renderEffectSelectorBar(px, py + selection*TINY_FONT_WIDTH, strlen(str[selection])*TINY_FONT_WIDTH);
		}

		displayScreen();
	} while(true);
}

uint16 shadeColor(uint16 c, int shade)	// dark=0, bright=256
{
	const int rOrig = (c >> 10) & 31;
	const int gOrig = (c >> 5) & 31;
	const int bOrig = c & 31;

	const int rShaded = (rOrig * shade) >> 8;
	const int gShaded = (gOrig * shade) >> 8;
	const int bShaded = (bOrig * shade) >> 8;

	return (rShaded << 10) | (gShaded << 5) | bShaded;
}

void setPal(int c, int r, int g, int b, uint16* pal)
{
	CLAMP(r, 0, 31)
	CLAMP(g, 0, 31)
	CLAMP(b, 0, 31)

	pal[c] = (r << 10) | (g << 5) | b;
}

void setPalGradient(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16* pal)
{
	int i;
	const int dc = (c1 - c0);
	const int dr = ((r1 - r0) << 16) / dc;
	const int dg = ((g1 - g0) << 16) / dc;
	const int db = ((b1 - b0) << 16) / dc;

	r0 <<= 16;
	g0 <<= 16;
	b0 <<= 16;

	for (i = c0; i <= c1; i++)
	{
		setPal(i, r0>>16, g0>>16, b0>>16, pal);

		r0 += dr;
		g0 += dg;
		b0 += db;
	}
}

void setPalGradientFromPrevIndex(int c0, int c1, int r1, int g1, int b1, uint16* pal)
{
	int r0, g0, b0;

	c0--;
	r0 = (pal[c0] >> 10) & 31;
	g0 = (pal[c0] >> 5) & 31;
	b0 = pal[c0] & 31;

	setPalGradient(c0, c1, r0, g0, b0, r1, g1, b1, pal);
}

static int getVramOffset16(int posX, int posY)
{
	return ((posY & ~1) * SCREEN_WIDTH) + (posY & 1) + 2*posX;
}

void drawBorderEdges(int posX, int posY, int width, int height)
{
	int i;
	const uint16 col = MakeRGB15(31, 31, 31);
	uint16 *vram = getVramBuffer();

	const int off00 = getVramOffset16(posX, posY);
	const int off01 = getVramOffset16(posX, posY+height-1);

	for (i=0; i<width; ++i) {
		*(vram + off00 + 2*i) = col;
		*(vram + off01 + 2*i) = col;
	}

	for (i=1; i<height-1; ++i) {
		const int offY0 = getVramOffset16(posX, posY+i);
		const int offY1 = getVramOffset16(posX+width-1, posY+i);
		*(vram + offY0) = col;
		*(vram + offY1) = col;
	}
}

void printDebugNum(int num)
{
	if (debugIndex < MAX_DEBUG_NUMS) {
		debugNums[debugIndex] = num;
		++debugIndex;
	}
}

void displayDebugNums(bool resetAfter)
{
	int i;
	for (i=0; i<debugIndex; ++i) {
		const int xp = (i >> 4) * 64;
		const int yp = 48 + (i & 15) * 8;
		drawNumber(xp,yp, debugNums[i]);
	}
	if (resetAfter) debugIndex = 0;
}
