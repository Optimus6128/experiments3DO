#include "types.h"
#include "core.h"

#include "system_graphics.h"
#include "tools.h"

static bool vsync = true;

static Item gVRAMIOReq;
static Item vsyncItem;

static int vramBuffersNum = DEFAULT_VRAM_BUFFERS_NUM;
static int offscreenBuffersNum = DEFAULT_OFFSCREEN_BUFFERS_NUM;

static Item *BitmapItems;
static Item **BufferItems;
static Bitmap **Bitmaps;
static Bitmap **Buffers;

static ScreenContext screen;

static IOInfo ioInfo;

static int screenPage = 0;
static int frameNum = 0;

bool renderToBuffer = false;
static uint32 bufferIndex = 0;

void initSPORTwriteValue(uint32 value)
{
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = FLASHWRITE_CMD;
	ioInfo.ioi_CmdOptions = 0xffffffff;
	ioInfo.ioi_Offset = value; // background colour
	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[0]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;
}

void initSPORTcopyImage(ubyte *srcImage)
{
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = SPORTCMD_COPY;
	ioInfo.ioi_Offset = 0xffffffff; // mask
	ioInfo.ioi_Send.iob_Buffer = srcImage;
	ioInfo.ioi_Send.iob_Len = SCREEN_SIZE_IN_BYTES;
	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[0]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;
}

void initGraphics(uint32 numVramBuffers, uint32 numOffscreenBuffers, bool horizontalAntialiasing, bool verticalAntialiasing)
{
	int i;
	int width,height;
	uint32 totalBuffersNum;

	if (numVramBuffers !=0 || numOffscreenBuffers != 0) {	// if both vram and offscreen buffers are 0, we will use the defaults anyway
		vramBuffersNum = numVramBuffers;
		offscreenBuffersNum = numOffscreenBuffers;
	}
	totalBuffersNum = vramBuffersNum + offscreenBuffersNum;

	CreateBasicDisplay(&screen, DI_TYPE_DEFAULT, totalBuffersNum);   // DI_TYPE_DEFAULT = 0 (NTSC)

	BitmapItems = (Item*)AllocMem(sizeof(Item) * totalBuffersNum, MEMTYPE_ANY);
	BufferItems = (Item**)AllocMem(sizeof(Item*) * offscreenBuffersNum, MEMTYPE_ANY);
	Bitmaps = (Bitmap**)AllocMem(sizeof(Bitmap*) * totalBuffersNum, MEMTYPE_ANY);
	Buffers = (Bitmap**)AllocMem(sizeof(Bitmap*) * offscreenBuffersNum, MEMTYPE_ANY);

	for(i=0; i<totalBuffersNum; ++i) {
		BitmapItems[i] = screen.sc_BitmapItems[i];
		Bitmaps[i] = screen.sc_Bitmaps[i];

		SetCEControl(BitmapItems[i], 0xffffffff, ASCALL);	// Enable Hardware CEL clipping

		if (horizontalAntialiasing)	EnableHAVG(BitmapItems[i]);
		if (verticalAntialiasing) EnableVAVG(BitmapItems[i]);
	}

	for (i=0; i<offscreenBuffersNum; ++i) {
        Buffers[i] = Bitmaps[vramBuffersNum + i];
        BufferItems[i] = &BitmapItems[vramBuffersNum + i];
	}

	width = Bitmaps[0]->bm_Width;
	height = Bitmaps[0]->bm_Height;

	gVRAMIOReq = CreateVRAMIOReq(); // Obtain an IOReq for all SPORT operations

	initSPORTwriteValue(0);

	vsyncItem = GetVBLIOReq();
}

void loadAndSetBackgroundImage(char *path, ubyte *screenBuffer)
{
	initSPORTcopyImage(LoadImage(path, screenBuffer, (VdlChunk **)NULL, &screen));
}

void setBackgroundColor(int color)
{
	ioInfo.ioi_Offset = color;
}

uint16 *getVramBuffer()
{
    return (uint16*)Bitmaps[screenPage]->bm_Buffer;
}

uint16 *getBackBuffer()
{
    return (uint16*)Buffers[bufferIndex]->bm_Buffer;
}

uint32 getNumVramBuffers()
{
	return vramBuffersNum;
}

uint32 getNumOffscreenBuffers()
{
	return offscreenBuffersNum;
}

void switchBuffer(bool on)
{
    renderToBuffer = on;
}

void setBuffer(uint32 num)
{
    if (num > offscreenBuffersNum-1) num = offscreenBuffersNum-1;

    bufferIndex = num;
}

void drawPixel(int px, int py, uint16 c)
{
	uint16 *dst = (uint16*)(Bitmaps[screenPage]->bm_Buffer) + (py >> 1) * SCREEN_WIDTH * 2 + (py & 1) + (px << 1);
	*dst = c;
}

int getFrameNum()
{
	return frameNum;
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
	if (vsync && ioInfo.ioi_Command != SPORTCMD_COPY) WaitVBL(vsyncItem, 1);

	if (++screenPage >= vramBuffersNum) screenPage = 0;

	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[screenPage]->bm_Buffer;
	DoIO(gVRAMIOReq,&ioInfo);

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
