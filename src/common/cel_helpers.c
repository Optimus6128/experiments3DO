#include "cel_helpers.h"
#include "system_graphics.h"

static int bppTable[8] = {0, 1,2,4,6,8,16, 0};

static void setDefaultCelValues(CCB *cel)
{
	cel->ccb_Flags =	CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_LDPLUT |
						CCB_CCBPRE | CCB_YOXY | CCB_ACW | CCB_ACCW | CCB_ACE | CCB_USEAV | CCB_NOBLK;

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
	int type = 0;

	if (cel->ccb_PRE0 & PRE0_LINEAR) {
		type |= CEL_TYPE_UNCODED;
	}
	if (cel->ccb_Flags & CCB_PACKED) {
		type |= CEL_TYPE_PACKED;
	}

	return type;
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
	if (width < 1 || width > 2048) return;

	cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_TLHPCNT_MASK) | (width - PRE1_TLHPCNT_PREFETCH);
}

void setCelHeight(int height, CCB *cel)
{
	if (height < 1 || height > 1024) return;

	cel->ccb_PRE0 = (cel->ccb_PRE0 & ~PRE0_VCNT_MASK) | ((height - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT);
}

void setCelStride(int stride, CCB *cel)
{
	const int bpp = getCelBpp(cel);
	int woffset;

	if (bpp==0 || stride < 8 || stride > 2048) return;

	woffset = (stride >> 2) - PRE1_WOFFSET_PREFETCH;

	if (bpp < 8) {
		cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET8_MASK) | (woffset << PRE1_WOFFSET8_SHIFT);
	} else {
		cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET10_MASK) | (woffset << PRE1_WOFFSET10_SHIFT);
	}
}

static void initCelWidth(int width, CCB *cel)
{
	const int bpp = getCelBpp(cel);
	int woffset;

	if (bpp==0 || width < 1 || width > 2048) return;

	woffset = (((width * bpp) + 31) >> 5);
	if (woffset < 2) woffset = 2;	// woffset can be minimum 2 words (8 bytes)
	woffset -= PRE1_WOFFSET_PREFETCH;

	if (bpp < 8) {
		cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET8_MASK) | (woffset << PRE1_WOFFSET8_SHIFT);
	} else {
		cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET10_MASK) | (woffset << PRE1_WOFFSET10_SHIFT);
	}

	setCelWidth(width, cel);

	cel->ccb_Width = width;	// in the future in full replace we won't save this, Lib3DO functions need it right now!
}

static void initCelHeight(int height, CCB *cel)
{
	if (height < 1 || height > 1024) return;

	setCelHeight(height, cel);

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

	if (type & CEL_TYPE_UNCODED) { cel->ccb_PRE0 |= PRE0_LINEAR; cel->ccb_Flags &= ~CCB_LDPLUT; }
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

void setCelPosition(int x, int y, CCB *cel)
{
	cel->ccb_XPos = x << 16;
	cel->ccb_YPos = y << 16;
}

void flipCelOrientation(bool horizontal, bool vertical, CCB *cel)
{
	if (horizontal){
		cel->ccb_HDX = -cel->ccb_HDX;
		cel->ccb_HDY = -cel->ccb_HDY;
	}

	if (vertical) {
		cel->ccb_VDX = -cel->ccb_VDX;
		cel->ccb_VDY = -cel->ccb_VDY;
	}
}

void rotateCelOrientation(CCB *cel)
{
	int tempX;

	tempX = cel->ccb_HDX;
	cel->ccb_HDX = cel->ccb_HDY;
	cel->ccb_HDY = tempX;

	tempX = cel->ccb_VDX;
	cel->ccb_VDX = cel->ccb_VDY;
	cel->ccb_VDY = tempX;
}


void setCelFlags(CCB *cel, uint32 flags, bool enable)
{
	if (enable) {
		cel->ccb_Flags |= flags;
	} else {
		cel->ccb_Flags &= ~flags;
	}
}

void initCel(int width, int height, int bpp, int type, CCB *cel)
{
	if (!cel) return;

	setDefaultCelValues(cel);

	// Don't change the order of these four!
	setCelBpp(bpp, cel);
	setCelType(type, cel);
	initCelHeight(height, cel);
	initCelWidth(width, cel);

	if (type & CEL_TYPE_ALLOC_PAL) {
		int palSize = getCelPaletteColorsRealBpp(bpp) * sizeof(uint16);
		cel->ccb_PLUTPtr = (uint16*)AllocMem(palSize, MEMTYPE_ANY);
	}

	if (type & CEL_TYPE_ALLOC_BMP) {
		int bmpSize = getCelDataSizeInBytes(cel);
		cel->ccb_SourcePtr = AllocMem(bmpSize, MEMTYPE_ANY);
	}
}

CCB *createCel(int width, int height, int bpp, int type)
{
	CCB *cel = (CCB*)AllocMem(sizeof(CCB), MEMTYPE_ANY);

	initCel(width, height, bpp, type, cel);

	return cel;
}

CCB *createCels(int width, int height, int bpp, int type, int num)
{
	int i;

	CCB *cels = (CCB*)AllocMem(num * sizeof(CCB), MEMTYPE_ANY);

	for (i=0; i<num; ++i) {
		initCel(width, height, bpp, type, &cels[i]);
	}

	return cels;
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

int getCelPaletteColorsRealBpp(int bpp)
{
	if (bpp <= 4) {
		return bpp;
	}
	if (bpp <= 8) {
		return 5;
	}
	return 0;
}

void linkCel(CCB *ccb, CCB *nextCCB)
{
	if (!ccb || !nextCCB) return;

	ccb->ccb_NextPtr = nextCCB;
	ccb->ccb_Flags &= ~CCB_LAST;
}

void updateWindowCel(int posX, int posY, int width, int height, int *bitmap, CCB *cel)
{
	int xPos32, lineSize32;
	const int bpp = getCelBpp(cel);

	if (bpp==0 || width < 1 || width > 2048 || height < 1 || height > 2048) return;

	setCelWidth(width, cel);
	setCelHeight(height, cel);

	xPos32 = (posX * bpp) >> 5;
	lineSize32 = (cel->ccb_Width * bpp) >> 5;

	cel->ccb_SourcePtr = (CelData*)&bitmap[posY * lineSize32 + xPos32];
}

void setupWindowFeedbackCel(int posX, int posY, int width, int height, int bufferIndex, CCB *cel)
{
	int woffset;
	int vcnt;

	cel->ccb_Flags &= ~(CCB_ACSC | CCB_ALSC);	// Super Clipping will lock an LRFORM feedback texture. Disable it!
	cel->ccb_Flags |= CCB_BGND;
	cel->ccb_PRE1 |= PRE1_LRFORM;
	cel->ccb_SourcePtr = (CelData*)(getBackBufferByIndex(bufferIndex) + (posY & ~1) * SCREEN_WIDTH + 2*posX);

	woffset = SCREEN_WIDTH - PRE1_WOFFSET_PREFETCH;
	vcnt = (height >> 1) - PRE0_VCNT_PREFETCH;

	cel->ccb_PRE0 = (cel->ccb_PRE0 & ~PRE0_VCNT_MASK) | (vcnt << PRE0_VCNT_SHIFT);
	cel->ccb_PRE1 = (cel->ccb_PRE1 & ~PRE1_WOFFSET10_MASK) | (woffset << PRE1_WOFFSET10_SHIFT) | (width-PRE1_TLHPCNT_PREFETCH);
}
