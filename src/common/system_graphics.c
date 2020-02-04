#include "types.h"
#include "core.h"

#include "system_graphics.h"
#include "tools.h"

static bool vsync = true;

static Item gVRAMIOReq;
static Item vsyncItem;

static Item BitmapItems[NUM_SCREEN_PAGES];
static Bitmap *Bitmaps[NUM_SCREEN_PAGES];

static ScreenContext screen;

static IOInfo ioInfo;

static int screenPage = 0;
static int frameNum = 0;

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

void initGraphics()
{
	int i;
	int width,height;

	CreateBasicDisplay(&screen,DI_TYPE_DEFAULT,NUM_SCREEN_PAGES);   // DI_TYPE_DEFAULT = 0 (NTSC)

	for(i=0;i<NUM_SCREEN_PAGES;i++)
	{
		BitmapItems[i] = screen.sc_BitmapItems[i];
		Bitmaps[i] = screen.sc_Bitmaps[i];

		SetCEControl(BitmapItems[i], 0xffffffff, ASCALL);

		EnableHAVG( BitmapItems[i] );
		EnableVAVG( BitmapItems[i] );
	}

	width = Bitmaps[0]->bm_Width;
	height = Bitmaps[0]->bm_Height;

	gVRAMIOReq = CreateVRAMIOReq(); // Obtain an IOReq for all SPORT operations

	initSPORTwriteValue(0);

	vsyncItem = GetVBLIOReq();
}

void loadAndSetBackgroundImage(char *path)
{
	initSPORTcopyImage(LoadImage(path, NULL, (VdlChunk **)NULL, &screen));
}

void setBackgroundColor(int color)
{
	ioInfo.ioi_Offset = color;
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

	screenPage = (screenPage+ 1) & 1;

	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[screenPage]->bm_Buffer;
	DoIO(gVRAMIOReq,&ioInfo);

	++frameNum;
}

void drawCels(CCB *cels)
{
	DrawCels(BitmapItems[screenPage], cels);
}
