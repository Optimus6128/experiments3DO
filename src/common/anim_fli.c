#include "anim_fli.h"
#include "file_utils.h"
#include "tools.h"

static unsigned char *vga_screen = NULL;
static uint16 vga_pal[256];

static unsigned char *fliPreload;
static uint32 fliIndex = 0;

static uint32 nextFrameIndex = 0;
static uint32 nextchunk = 0;
static uint32 after_first_frame = 0;

static uint32 yline;

static FLIheader FLIhdr;
static FRAMEheader FRMhdr;
static CHUNKheader CHKhdr;

static bool shouldUpdateFullFrame;


static uint16 readU16()
{
	uint16 value;

	if ((fliIndex & 1) == 0) {
		value = *((uint16*)&fliPreload[fliIndex]);
		value = SHORT_ENDIAN_FLIP(value);
	} else {
		const uint32 u0 = fliPreload[fliIndex];
		const uint32 u1 = fliPreload[fliIndex+1];

		value = (uint16)( (u1 << 8) | u0 );
	}
	fliIndex += 2;

	return value;
}

static uint32 readU32()
{
	uint32 value;

	if ((fliIndex & 3) == 0) {
		value = *((uint32*)&fliPreload[fliIndex]);
		value = LONG_ENDIAN_FLIP(value);
	} else {
		const uint32 u0 = fliPreload[fliIndex];
		const uint32 u1 = fliPreload[fliIndex+1];
		const uint32 u2 = fliPreload[fliIndex+2];
		const uint32 u3 = fliPreload[fliIndex+3];

		value = (uint32)( (u3 << 24) | (u2 << 16) | (u1 << 8) | u0 );
	}
	fliIndex += 4;

	return value;
}

void ReadFrameHDR()
{
	FRMhdr.size = readU32();
	nextFrameIndex = fliIndex - 4 + FRMhdr.size;

	FRMhdr.magic = readU16();
	FRMhdr.chunks = readU16();

	memcpy(FRMhdr.expand, fliPreload, 8);
	fliIndex += 8;
}

void ReadChunkHDR()
{
	CHKhdr.size = readU32();
	nextchunk = fliIndex - 4 + CHKhdr.size;
	CHKhdr.type = readU16();
}


void FliColor()
{
	int i, j, ci=0;

	const uint16 packets = readU16();

	for (i=0; i<packets; i++)
	{
		const int colors2skip = fliPreload[fliIndex++];
		int colors2chng = fliPreload[fliIndex++];

		if (colors2chng==0) colors2chng = VGA_PAL_SIZE;
		ci+=colors2skip;
		for (j=0; j<colors2chng; j++)
		{
			const int r = (fliPreload[fliIndex++]>>1);
			const int g = (fliPreload[fliIndex++]>>1);
			const int b = (fliPreload[fliIndex++]>>1);
			vga_pal[ci++] = (r<<10) | (g<<5) | b;
		}
	}
}


void FliBrun()
{
	int i, j, y, vi=0;

	for (y=0; y<FLIhdr.height; y++) {
		const unsigned char packets = fliPreload[fliIndex++];
		for (i=0; i<packets; i++) {
			int8 size_count = (int8)fliPreload[fliIndex++];
			const unsigned char data = fliPreload[fliIndex++];

			if (size_count>=0) {
				for (j=0; j<size_count; j++) {
					vga_screen[vi++] = data;
				}
			} else {
				size_count = -size_count;

				vga_screen[vi++] = data;
				for (j=1; j<size_count; j++) {
					vga_screen[vi++] = fliPreload[fliIndex++];
				}
			}
		}
	}
}

void FliLc(AnimFLI *anim)
{
	int i, j, n, vi=0;
	uint16 *dst = anim->bmp;

	const uint16 lines_skip = readU16();
	const uint16 lines_chng = readU16();

	yline += lines_skip;
	vi = yline * VGA_WIDTH;

	for (i=0; i<lines_chng; i++) {
		const unsigned char packets = fliPreload[fliIndex++];

		for (n=0; n<packets; n++) {
			const unsigned char skip_count = fliPreload[fliIndex++];
			int8 size_count = fliPreload[fliIndex++];

			vi+=skip_count;
			if (size_count<0) {
				const unsigned char data = fliPreload[fliIndex++];
				size_count = -size_count;
				for (j=0; j<size_count; j++) {
					vga_screen[vi] = data;
					dst[vi] = vga_pal[data];
					vi++;
				}
			} else {
				for (j=0; j<size_count; j++) {
					const unsigned char data = fliPreload[fliIndex++];
					vga_screen[vi] = data;
					dst[vi] = vga_pal[data];
					vi++;
				}
			}
		}	
		yline++;
		vi = yline * VGA_WIDTH;
	}
}

void FliCopy()
{
	memcpy(vga_screen, &fliPreload[fliIndex], VGA_SIZE);
	fliIndex+=VGA_SIZE;
}

void FliBlack()
{
	memset(vga_screen, 0, VGA_SIZE);
	memset(vga_pal, 0, 2 * VGA_PAL_SIZE);
}

void DoType(uint16 type, AnimFLI *anim)
{
	switch(type)
	{
		case FLI_COLOR:
			FliColor();
			shouldUpdateFullFrame = true;
		break;

		case FLI_BRUN:
			after_first_frame = nextFrameIndex;
			FliBrun();
			shouldUpdateFullFrame = true;
		break;

		case FLI_LC:
			FliLc(anim);
		break;

		case FLI_BLACK:
			FliBlack();
			shouldUpdateFullFrame = true;
		break;

		case FLI_COPY:
			FliCopy();
			shouldUpdateFullFrame = true;
		break;

//		case FLI_256_COLOR:
//		break;

//		case FLI_DELTA:
//		break;

//		case FLI_MINI:
//		break;

		default:
			//printf("%d\n",CHKhdr.type);
		break;
	}
}

void FLIupdateFullFrame(AnimFLI *anim)
{
	int count = (VGA_WIDTH * VGA_HEIGHT) / 4;

	uint16 *dst = anim->bmp;
	uint32 *vga32 = (uint32*)vga_screen;
	do {
		const uint32 c = *vga32++;

		*dst++ = vga_pal[(c >> 24) & 255];
		*dst++ = vga_pal[(c >> 16) & 255];
		*dst++ = vga_pal[(c >> 8) & 255];
		*dst++ = vga_pal[c & 255];
	} while(--count > 0);
}

void FLIplayNextFrame(AnimFLI *anim)
{
	int i;

	if (vga_screen==NULL) vga_screen = (unsigned char*)AllocMem(64000, MEMTYPE_ANY);
	shouldUpdateFullFrame = false;

	ReadFrameHDR();
	yline=0;

	for (i=0; i<FRMhdr.chunks; i++)
	{
		ReadChunkHDR();
		DoType(CHKhdr.type, anim);
		fliIndex = nextchunk;
	}
	fliIndex = nextFrameIndex;

	if (fliIndex >= FLIhdr.size-sizeof(FLIhdr)) {
		nextFrameIndex = fliIndex = after_first_frame;
	}

	if (shouldUpdateFullFrame) {
		FLIupdateFullFrame(anim);
	}
}

void FLIload(AnimFLI *anim)
{
	char *filename = anim->filename;

	readBytesFromFileAndStore(filename, 0, sizeof(FLIhdr), (char*)&FLIhdr);

	FLIhdr.size = LONG_ENDIAN_FLIP(FLIhdr.size);
	FLIhdr.width = SHORT_ENDIAN_FLIP(FLIhdr.width);
	FLIhdr.height = SHORT_ENDIAN_FLIP(FLIhdr.height);

    FLIhdr.magic = SHORT_ENDIAN_FLIP(FLIhdr.magic);
    FLIhdr.frames = SHORT_ENDIAN_FLIP(FLIhdr.frames);

    FLIhdr.depth = SHORT_ENDIAN_FLIP(FLIhdr.depth);
    FLIhdr.flags = SHORT_ENDIAN_FLIP(FLIhdr.flags);
    FLIhdr.speed = SHORT_ENDIAN_FLIP(FLIhdr.speed);

    FLIhdr.next = LONG_ENDIAN_FLIP(FLIhdr.next);
    FLIhdr.frit = LONG_ENDIAN_FLIP(FLIhdr.frit);

	fliPreload = AllocMem(FLIhdr.size, MEMTYPE_ANY);

	readBytesFromFileAndStore(filename, sizeof(FLIhdr), FLIhdr.size - sizeof(FLIhdr), fliPreload);
}

AnimFLI *newAnimFLI(char *filename, uint16 *bmp)
{
	AnimFLI *anim = AllocMem(sizeof(AnimFLI), MEMTYPE_ANY);

	anim->filename = filename;
	anim->bmp = bmp;

	return anim;
}
