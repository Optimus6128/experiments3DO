#include "types.h"
#include "main_includes.h"

#include "system_graphics.h"
#include "tools.h"

bool vsync = true;

static Item gVRAMIOReq;
static Item vsyncItem;

static Item BitmapItems[NUM_SCREEN_PAGES];
static Bitmap *Bitmaps[NUM_SCREEN_PAGES];

static ScreenContext screen;

static IOInfo ioInfo;

static int screenPage = 0;

void initGraphics()
{
	int i;
	int width,height;

	//int test = OpenGraphics(&screen,NUM_SCREEN_PAGES);
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

	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = FLASHWRITE_CMD;
	ioInfo.ioi_CmdOptions = 0xffffffff;
	ioInfo.ioi_Offset = 0x32122123;//0x12344321; // background colour
	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[0]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = width*height*2;   // 2 could be because 16bit and not because number of buffers, gotta check

	vsyncItem = GetVBLIOReq();
}

void debugUpdate()
{
	static int incNum = 0;
	int i;

	int c = rand() & 65535;
return;
	ioInfo.ioi_Offset = (c << 16) | c;

	for (i=0; i<10; ++i) {
		drawNumber(0, 0, incNum);
		displayScreen();
	}

	ioInfo.ioi_Offset = 0;
	++incNum;
}

void drawPixel(int px, int py, uint16 c)
{
	uint16 *dst = (uint16*)(Bitmaps[screenPage]->bm_Buffer) + (py >> 1) * SCREEN_WIDTH * 2 + (py & 1) + (px << 1);
	*dst = c;
}

void displayScreen()
{
	DisplayScreen(screen.sc_Screens[screenPage], 0 );
	if (vsync) WaitVBL(vsyncItem, 1);

	screenPage = (screenPage+ 1) & 1;

	ioInfo.ioi_Recv.iob_Buffer = Bitmaps[screenPage]->bm_Buffer;
	DoIO(gVRAMIOReq,&ioInfo);
}

void drawCels(CCB *cels)
{
	DrawCels(BitmapItems[screenPage], cels);
}
