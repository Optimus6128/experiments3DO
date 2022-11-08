#include "anim_fli.h"
#include "file_utils.h"

static unsigned char *vga_screen = NULL;
static uint16 vga_pal[256];

static unsigned char *fliPreload;
static uint32 fliIndex = 0;

static uint32 nextFliIndex = 0;
static uint32 nextchunk = 0;
static uint32 after_first_frame = 0;

static uint32 yline;

static FLIheader FLIhdr;
static FRAMEheader FRMhdr;
static CHUNKheader CHKhdr;

static int nope = 0;

static uint16 readU16()
{
	const uint16 value = *((uint16*)&fliPreload[fliIndex]);
	fliIndex += 2;
	return SHORT_ENDIAN_FLIP(value);
}

static uint32 readU32()
{
	const uint32 value = *((uint32*)&fliPreload[fliIndex]);
	fliIndex += 4;
	return LONG_ENDIAN_FLIP(value);
}

void ReadFrameHDR()
{
	FRMhdr.size = readU32();
	nextFliIndex = fliIndex - 4 + FRMhdr.size;

	FRMhdr.magic = readU16();
	FRMhdr.chunks = readU16();

	nope = FRMhdr.magic;

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

		if (colors2chng==0) colors2chng = 256;
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


void FliLc()
{
	int i, j, n, vi=0;

	const uint16 lines_skip = readU16();
	const uint16 lines_chng = readU16();

	yline += lines_skip;
	vi = yline*(VGA_WIDTH>>1);

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
					vga_screen[vi++] = data;
				}
			} else {
				for (j=0; j<size_count; j++) {
					vga_screen[vi++] = fliPreload[fliIndex++];
				}
			}
		}	
		yline++;
		vi = yline*(VGA_WIDTH>>1);
	}
}

void FliCopy()
{
	memcpy(vga_screen, &fliPreload[fliIndex], 64000);
	fliIndex+=64000;
}

void FliBlack()
{
	memset(vga_screen, 0, 64000);
	//memset(vga_pal, 0, 512);
}

void DoType(uint16 type)
{
	switch(type)
	{
		case FLI_COLOR:
			FliColor();
		break;

		case FLI_BRUN:
			after_first_frame = nextFliIndex;
			FliBrun();
		break;

		case FLI_LC:
			FliLc();
		break;

		case FLI_BLACK:
			FliBlack();
		break;

		case FLI_COPY:
			FliCopy();
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

void FLIplayNextFrame(AnimFLI *anim)
{
	int i;

	if (vga_screen==NULL) vga_screen = (unsigned char*)AllocMem(64000, MEMTYPE_ANY);

	ReadFrameHDR();
	yline=0;

	for (i=0; i<FRMhdr.chunks; i++)
	{
		ReadChunkHDR();
		DoType(CHKhdr.type);
		fliIndex = nextchunk;
	}
	fliIndex = nextFliIndex;

	if (fliIndex >= FLIhdr.size-128) {
		nextFliIndex = fliIndex = after_first_frame;
	}
}

void FLIshow(AnimFLI *anim, uint16 *dst)
{
	int count = (VGA_WIDTH * VGA_HEIGHT) / 4;

	uint32 *vga32 = (uint32*)vga_screen;
	do {
		const uint32 c = *vga32++;

		*dst++ = nope;
		*dst++ = nope;
		*dst++ = nope;
		*dst++ = nope;

		//*dst++ = (c >> 24) & 255;
		//*dst++ = (c >> 16) & 255;
		//*dst++ = (c >> 8) & 255;
		//*dst++ = c & 255;

		//*dst++ = vga_pal[(c >> 24) & 255];
		//*dst++ = vga_pal[(c >> 16) & 255];
		//*dst++ = vga_pal[(c >> 8) & 255];
		//*dst++ = vga_pal[c & 255];
	} while(--count > 0);
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

	readBytesFromFileAndStore(filename, sizeof(FLIhdr), FLIhdr.size - 128, fliPreload);
}

AnimFLI *newAnimFLI(char *filename)
{
	AnimFLI *anim = AllocMem(sizeof(AnimFLI), MEMTYPE_ANY);

	anim->filename = filename;

	return anim;
}
