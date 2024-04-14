#include "types.h"
#include "core.h"

#include "system_graphics.h"
#include "tools.h"

static bool vsync;
static bool clearFrame;

static Item gVRAMIOReq;
static Item vsyncItem;

static int vramBuffersNum;
static int offscreenBuffersNum;

static Item *BitmapItems;
static Item **BufferItems;
static Bitmap **Bitmaps;
static Bitmap **Buffers;

static ScreenContext screen;

static IOInfo ioInfo;

static int screenPage;
static int frameNum;
bool renderToBuffer;
static uint32 bufferIndex;

static void *lastSPORTimage = NULL;
static uint32 lastSPORTcolor = 0;

void initSPORTwriteValue(uint32 value)
{
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = FLASHWRITE_CMD;
	ioInfo.ioi_CmdOptions = 0xffffffff;
	ioInfo.ioi_Offset = value; // background colour
	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[0]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;

	lastSPORTcolor = value;
}

void initSPORTcopyImage(unsigned char *srcImage)
{
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = SPORTCMD_COPY;
	ioInfo.ioi_Offset = 0xffffffff; // mask
	ioInfo.ioi_Send.iob_Buffer = srcImage;
	ioInfo.ioi_Send.iob_Len = SCREEN_SIZE_IN_BYTES;
	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[0]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;

	lastSPORTimage = srcImage;
}

static bool testIfTotalBuffersFitInVRAM(uint32 totalNumBuffers)
{
	MemInfo memInfoVRAM;

	AvailMem(&memInfoVRAM, MEMTYPE_VRAM);
	return (totalNumBuffers * SCREEN_SIZE_IN_BYTES < memInfoVRAM.minfo_SysFree);
}

void initGraphics(uint32 numVramBuffers, uint32 numOffscreenBuffers, bool horizontalAntialiasing, bool verticalAntialiasing)
{
	int i;
	uint32 totalBuffersNum;

	screenPage = 0;
	frameNum = 0;
	bufferIndex = 0;
	renderToBuffer = false;

	vramBuffersNum = DEFAULT_VRAM_BUFFERS_NUM;
	offscreenBuffersNum = DEFAULT_OFFSCREEN_BUFFERS_NUM;

	if (numVramBuffers !=0 || numOffscreenBuffers != 0) {	// if both vram and offscreen buffers are 0, we will use the defaults anyway
		vramBuffersNum = numVramBuffers;
		offscreenBuffersNum = numOffscreenBuffers;
	}

	totalBuffersNum = vramBuffersNum + offscreenBuffersNum;
	while (!testIfTotalBuffersFitInVRAM(totalBuffersNum) && totalBuffersNum!=0) {
		totalBuffersNum--;
		offscreenBuffersNum = totalBuffersNum - vramBuffersNum;
		if (offscreenBuffersNum < 0) {
			offscreenBuffersNum = 0;
			vramBuffersNum--;
		}
	}

	if (totalBuffersNum > 0) {
		CreateBasicDisplay(&screen, DI_TYPE_DEFAULT, totalBuffersNum);	// DI_TYPE_DEFAULT = 0 (NTSC)
	} else {
		return; // else something went wrong
	}


	BitmapItems = (Item*)AllocMem(sizeof(Item) * totalBuffersNum, MEMTYPE_TRACKSIZE);
	BufferItems = (Item**)AllocMem(sizeof(Item*) * offscreenBuffersNum, MEMTYPE_TRACKSIZE);
	Bitmaps = (Bitmap**)AllocMem(sizeof(Bitmap*) * totalBuffersNum, MEMTYPE_TRACKSIZE);
	Buffers = (Bitmap**)AllocMem(sizeof(Bitmap*) * offscreenBuffersNum, MEMTYPE_TRACKSIZE);

	for(i=0; i<(int)totalBuffersNum; ++i) {
		BitmapItems[i] = screen.sc_BitmapItems[i];
		Bitmaps[i] = screen.sc_Bitmaps[i];

		memset(Bitmaps[i]->bm_Buffer, 0, Bitmaps[i]->bm_Width * Bitmaps[i]->bm_Height * 2);

		SetCEControl(BitmapItems[i], 0xffffffff, ASCALL);	// Enable Hardware CEL clipping

		if (horizontalAntialiasing)	EnableHAVG(BitmapItems[i]);
		if (verticalAntialiasing) EnableVAVG(BitmapItems[i]);
	}

	for (i=0; i<offscreenBuffersNum; ++i) {
		Buffers[i] = Bitmaps[vramBuffersNum + i];
		BufferItems[i] = &BitmapItems[vramBuffersNum + i];
	}

	gVRAMIOReq = CreateVRAMIOReq(); // Obtain an IOReq for all SPORT operations

	initSPORTwriteValue(lastSPORTcolor);

	vsyncItem = GetVBLIOReq();
}

void deInitGraphics()
{
	const uint32 totalBuffersNum = vramBuffersNum + offscreenBuffersNum;

	if (totalBuffersNum > 0) {
		FreeMem(BitmapItems, -1);
		FreeMem(BufferItems, -1);
		FreeMem(Bitmaps, -1);
		FreeMem(Buffers, -1);

		DeleteBasicDisplay(&screen);
		DeleteVBLIOReq(vsyncItem);
		DeleteVRAMIOReq(gVRAMIOReq);
	}
}

void loadAndSetBackgroundImage(char *path, void *screenBuffer)
{
	initSPORTcopyImage(LoadImage(path, screenBuffer, (VdlChunk **)NULL, &screen));
}

void setBackgroundColor(int color)
{
	ioInfo.ioi_Offset = color;
	lastSPORTcolor = color;
}

void switchToSPORTwrite()
{
	initSPORTwriteValue(lastSPORTcolor);
}

void switchToSPORTimage()
{
	if (lastSPORTimage) {
		initSPORTcopyImage(lastSPORTimage);
	}
}

uint16 *getVramBuffer()
{
	return (uint16*)Bitmaps[screenPage]->bm_Buffer;
}

uint16 *getBackBuffer()
{
	return (uint16*)Buffers[bufferIndex]->bm_Buffer;
}

uint16 *getBackBufferByIndex(int index)
{
	if (index > offscreenBuffersNum-1) index = offscreenBuffersNum-1;

	return (uint16*)Buffers[index]->bm_Buffer;
}

uint32 getNumVramBuffers()
{
	return vramBuffersNum;
}

uint32 getNumOffscreenBuffers()
{
	return offscreenBuffersNum;
}

void switchRenderToBuffer(bool on)
{
	renderToBuffer = on;
}

void setRenderBuffer(uint32 num)
{
	if ((int)num > offscreenBuffersNum-1) num = offscreenBuffersNum-1;

	bufferIndex = num;
}

void clearAllBuffers()
{
	const uint32 totalBuffersNum = vramBuffersNum + offscreenBuffersNum;
	uint32 i;

	for(i=0; i<totalBuffersNum; ++i) {
		memset(Bitmaps[i]->bm_Buffer, 0, Bitmaps[i]->bm_Width * Bitmaps[i]->bm_Height * 2);
	}
}

void clearBackBuffer()
{
	ioInfo.ioi_Recv.iob_Buffer = getBackBuffer();
	DoIO(gVRAMIOReq,&ioInfo);
}

void drawPixel(int px, int py, uint16 c)
{
	const int offset = (py >> 1) * SCREEN_WIDTH * 2 + (py & 1) + (px << 1);
	if (offset >= 0 && offset < SCREEN_WIDTH * SCREEN_HEIGHT) {
		uint16 *dst = (uint16*)(Bitmaps[screenPage]->bm_Buffer) + offset;
		*dst = c;
	}
}

void drawThickPixel(int px, int py, uint16 c)
{
	const uint32 cc = (c << 16) | c;
	uint32 *dst = (uint32*)((uint16*)Bitmaps[screenPage]->bm_Buffer + py * SCREEN_WIDTH * 2 + (px << 2));
	*dst++ = cc;
	*dst++ = cc;
}

int getFrameNum()
{
	return frameNum;
}

void setClearFrame(bool on)
{
	clearFrame = on;
}

void setVsync(bool on)
{
	vsync = on;
}

void toggleVsync()
{
	vsync = !vsync;
}

void displayScreen()
{
	DisplayScreen(screen.sc_Screens[screenPage], 0 );
	if (vsync && !(ioInfo.ioi_Command == SPORTCMD_COPY && clearFrame)) WaitVBL(vsyncItem, 1);

	if (++screenPage >= vramBuffersNum) screenPage = 0;

	if (clearFrame) {
		ioInfo.ioi_Recv.iob_Buffer = Bitmaps[screenPage]->bm_Buffer;
		DoIO(gVRAMIOReq,&ioInfo);
	}

	++frameNum;
}

void drawCels(CCB *cels)
{
	if (renderToBuffer) {
		DrawCels(*BufferItems[bufferIndex], cels);
	} else {
		DrawCels(BitmapItems[screenPage], cels);
	}
}
