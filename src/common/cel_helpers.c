#include "cel_helpers.h"
#include "system_graphics.h"

static int bppTable[8] = {0, 1,2,4,6,8,16, 0};

static void setDefaultCelValues(CCB *cel)
{
	cel->ccb_Flags =	CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_LDPLUT |
						CCB_CCBPRE | CCB_YOXY | CCB_ACW | CCB_ACCW | CCB_ACE | CCB_USEAV | CCB_POVER_MASK | CCB_NOBLK;

	// If we want superclipping (should do functions to set extra gimmicks)
	// cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);


	cel->ccb_NextPtr = NULL;
	cel->ccb_SourcePtr = NULL;
	cel->ccb_PLUTPtr = NULL;

	cel->ccb_XPos = 0;
	cel->ccb_YPos = 0;
	cel->ccb_HDX = 1 << 20;
	cel->ccb_HDY = 0;
	cel->ccb_VDX = 0;
	cel->ccb_VDY = 1 << 16;
	cel->ccb_HDDX = 0;
	cel->ccb_HDDY = 0;


	cel->ccb_PIXC = CEL_BLEND_OPAQUE;

	cel->ccb_PRE0 = 0;
	cel->ccb_PRE1 = PRE1_TLLSB_PDC0;	// blue LSB bit is blue
}

int getCelWidth(CCB *cel)
{
	return (cel->ccb_PRE1 & PRE1_TLHPCNT_MASK) + PRE1_TLHPCNT_PREFETCH;
}

int getCelHeight(CCB *cel)
{
	return ((cel->ccb_PRE0 & PRE0_VCNT_MASK) >> PRE0_VCNT_SHIFT) + PRE0_VCNT_PREFETCH;
}

int getCelBpp(CCB *cel)
{
	return bppTable[cel->ccb_PRE0 & PRE0_BPP_MASK];
}

int getCelType(CCB *cel)
{
	// will implement in the future
	return 0;
}

uint16* getCelPalette(CCB *cel)
{
	return cel->ccb_PLUTPtr;
}

void* getCelBitmap(CCB *cel)
{
	return cel->ccb_SourcePtr;
}


void setCelWidth(int width, CCB *cel)
{
	const int bpp = getCelBpp(cel);
	int woffset;

	if (bpp==0 || width < 1 || width > 2048) return;

	woffset = (((width * bpp) + 31) >> 5);
	if (woffset < 2) woffset = 2;	// woffset can be minimum 2 words (8 bytes)
	woffset -= PRE1_WOFFSET_PREFETCH;

	cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_TLHPCNT_MASK) | (width - PRE1_TLHPCNT_PREFETCH);
	if (bpp < 8) {
		cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET8_MASK) | (woffset << PRE1_WOFFSET8_SHIFT);
	} else {
		cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET10_MASK) | (woffset << PRE1_WOFFSET10_SHIFT);
	}

	cel->ccb_Width = width;	// in the future in full replace we won't save this, Lib3DO functions need it right now!
}

void setCelHeight(int height, CCB *cel)
{
	if (height < 1 || height > 1024) return;

	cel->ccb_PRE0 = (cel->ccb_PRE0 & ~PRE0_VCNT_MASK) | ((height - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT);

	cel->ccb_Height = height;	// in the future in full replace we won't save this, Lib3DO functions need it right now!
}

void setCelBpp(int bpp, CCB *cel)
{
	cel->ccb_PRE0 = (cel->ccb_PRE0 & ~PRE0_BPP_MASK) | bpp;
	if (bpp>0 && bpp<=16) {
		int i;
		for (i=1; i<7; ++i) {
			if (bpp == bppTable[i]) {
				cel->ccb_PRE0 = (cel->ccb_PRE0 & ~PRE0_BPP_MASK) | i;
			}
		}
	}
	// failed to find bpp in the table
}

void setCelType(int type, CCB *cel)
{
	cel->ccb_PRE0 &= ~PRE0_LINEAR;
	cel->ccb_Flags &= ~CCB_PACKED;

	if (type & CEL_TYPE_UNCODED) cel->ccb_PRE0 |= PRE0_LINEAR;
	if (type & CEL_TYPE_PACKED) cel->ccb_Flags |= CCB_PACKED;
}

void setCelPalette(uint16 *pal, CCB *cel)
{
	cel->ccb_PLUTPtr = pal;
}

void setCelBitmap(void *bitmap, CCB *cel)
{
	cel->ccb_SourcePtr = (CelData*)bitmap;
}

void initCel(int width, int height, int bpp, int type, CCB *cel)
{
	if (!cel) return;

	setDefaultCelValues(cel);

	// Don't change the order of these four!
	setCelBpp(bpp, cel);
	setCelType(type, cel);
	setCelHeight(height, cel);
	setCelWidth(width, cel);
}

CCB *createCel(int width, int height, int bpp, int type)
{
	CCB *cel = (CCB*)AllocMem(sizeof(CCB), MEMTYPE_ANY);

	initCel(width, height, bpp, type, cel);

	return cel;
}

void setupCelData(uint16 *pal, void *bitmap, CCB *cel)
{
	setCelPalette(pal, cel);
	setCelBitmap(bitmap, cel);
}

int getCelDataSizeInBytes(CCB *cel)
{
	int bpp = getCelBpp(cel);
	if (bpp == 6) bpp = 8;

	// doesn't account yet for packed cels or tiny cels (less than 8 bytes row width) with padding
	return (cel->ccb_Width * cel->ccb_Height * bpp) >> 3;
}

/*
// Will finish later. Ness more, bpp, skipping pixels depending on bpp, etc..
// It would be also nice to give the original CCB, get the max width/height from the extra structure that is not loaded to the hardware, get the bpp and other info from the bits
void setupWindowNormalCel(int posX, int posY, int width, int height, int totalWidth, int totalHeight, void *bitmap, CCB *cel)
{
	int woffset;
	int vcnt;

	cel->ccb_PRE1 &= ~PRE1_LRFORM;
	cel->ccb_SourcePtr = (CelData*)bitmap;
	woffset = (width >> 1) - 2;
	vcnt = height - 1;

	// Should spare the magic numbers at some point
	cel->ccb_PRE0 = (cel->ccb_PRE0 & ~(((1<<10) - 1)<<6)) | (vcnt << 6);
	cel->ccb_PRE1 = (cel->ccb_PRE1 & (65536 - 1024)) | (woffset << 16) | (width-1);
}*/

void setupWindowFeedbackCel(int posX, int posY, int width, int height, int bufferIndex, CCB *cel)
{
	int woffset;
	int vcnt;

	cel->ccb_Flags &= ~(CCB_ACSC | CCB_ALSC);	// Super Clipping will lock an LRFORM feedback texture. Disable it!
	cel->ccb_Flags |= CCB_BGND;
	cel->ccb_PRE1 |= PRE1_LRFORM;
	cel->ccb_SourcePtr = (CelData*)(getBackBufferByIndex(bufferIndex) + (posY & ~1) * SCREEN_WIDTH + 2*posX);
	woffset = SCREEN_WIDTH - 2;
	vcnt = (height >> 1) - 1;

	// Should spare the magic numbers at some point
	cel->ccb_PRE0 = (cel->ccb_PRE0 & ~(((1<<10) - 1)<<6)) | (vcnt << 6);
	cel->ccb_PRE1 = (cel->ccb_PRE1 & (65536 - 1024)) | (woffset << 16) | (width-1);
}
