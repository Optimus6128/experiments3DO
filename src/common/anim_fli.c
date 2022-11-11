#include "anim_fli.h"
#include "file_utils.h"
#include "tools.h"


static bool shouldUpdateFullFrame;


static uint16 readU16(AnimFLI *anim)
{
	uint16 value;
	const uint32 fliIndex = anim->fliIndex;
	unsigned char *fliPreload = anim->fliPreload;

	if ((fliIndex & 1) == 0) {
		value = *((uint16*)&fliPreload[fliIndex]);
		value = SHORT_ENDIAN_FLIP(value);
	} else {
		const uint32 u0 = fliPreload[fliIndex];
		const uint32 u1 = fliPreload[fliIndex+1];

		value = (uint16)( (u1 << 8) | u0 );
	}
	anim->fliIndex += 2;

	return value;
}

static uint32 readU32(AnimFLI *anim)
{
	uint32 value;
	const uint32 fliIndex = anim->fliIndex;
	unsigned char *fliPreload = anim->fliPreload;

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
	anim->fliIndex += 4;

	return value;
}

void ReadFrameHDR(AnimFLI *anim)
{
	anim->FRMhdr.size = readU32(anim);
	anim->nextFrameIndex = anim->fliIndex - 4 + anim->FRMhdr.size;

	anim->FRMhdr.magic = readU16(anim);
	anim->FRMhdr.chunks = readU16(anim);

	memcpy(anim->FRMhdr.expand, anim->fliPreload, 8);
	anim->fliIndex += 8;
}

void ReadChunkHDR(AnimFLI *anim)
{
	anim->CHKhdr.size = readU32(anim);
	anim->nextchunk = anim->fliIndex - 4 + anim->CHKhdr.size;
	anim->CHKhdr.type = readU16(anim);
}


void FliColor(AnimFLI *anim)
{
	int i, j, ci=0;
	unsigned char *fliPreload = anim->fliPreload;
	uint32 *vga_pal = anim->vga_pal;

	const uint16 packets = readU16(anim);

	for (i=0; i<packets; i++)
	{
		const int colors2skip = fliPreload[anim->fliIndex++];
		int colors2chng = fliPreload[anim->fliIndex++];

		if (colors2chng==0) colors2chng = VGA_PAL_SIZE;
		ci+=colors2skip;
		for (j=0; j<colors2chng; j++)
		{
			const int r = (fliPreload[anim->fliIndex++]>>1);
			const int g = (fliPreload[anim->fliIndex++]>>1);
			const int b = (fliPreload[anim->fliIndex++]>>1);
			vga_pal[ci++] = (r<<(10+PAL_PAD_BITS)) | (g<<(5+PAL_PAD_BITS)) | (b << PAL_PAD_BITS);
		}
	}
}


void FliBrun(AnimFLI *anim)
{
	int i, j, y, vi=0;
	unsigned char *fliPreload = anim->fliPreload;
	unsigned char *vga_screen = anim->vga_screen;

	anim->after_first_frame = anim->nextFrameIndex;

	for (y=0; y<anim->FLIhdr.height; y++) {
		const unsigned char packets = fliPreload[anim->fliIndex++];
		for (i=0; i<packets; i++) {
			int8 size_count = (int8)fliPreload[anim->fliIndex++];
			const unsigned char data = fliPreload[anim->fliIndex++];

			if (size_count>=0) {
				for (j=0; j<size_count; j++) {
					vga_screen[vi++] = data;
				}
			} else {
				size_count = -size_count;

				vga_screen[vi++] = data;
				for (j=1; j<size_count; j++) {
					vga_screen[vi++] = fliPreload[anim->fliIndex++];
				}
			}
		}
	}
}

void FliLc(AnimFLI *anim)
{
	int i, j, n, vi=0;

	uint16 *dst = anim->bmp;
	unsigned char *fliPreload = anim->fliPreload;
	unsigned char *vga_screen = anim->vga_screen;
	uint32 *vga_pal = anim->vga_pal;

	const uint16 lines_skip = readU16(anim);
	const uint16 lines_chng = readU16(anim);

	anim->yline += lines_skip;
	vi = anim->yline * VGA_WIDTH;

	for (i=0; i<lines_chng; i++) {
		const unsigned char packets = fliPreload[anim->fliIndex++];

		for (n=0; n<packets; n++) {
			const unsigned char skip_count = fliPreload[anim->fliIndex++];
			int8 size_count = fliPreload[anim->fliIndex++];

			vi+=skip_count;
			if (size_count<0) {
				const unsigned char data = fliPreload[anim->fliIndex++];
				const uint16 col = vga_pal[data] >> PAL_PAD_BITS;
				size_count = -size_count;
				for (j=0; j<size_count; j++) {
					vga_screen[vi] = data;
					dst[vi] = col;
					vi++;
				}
			} else {
				for (j=0; j<size_count; j++) {
					const unsigned char data = fliPreload[anim->fliIndex++];
					const uint16 col = vga_pal[data] >> PAL_PAD_BITS;
					vga_screen[vi] = data;
					dst[vi] = col;
					vi++;
				}
			}
		}	
		anim->yline++;
		vi = anim->yline * VGA_WIDTH;
	}
}

void FliCopy(AnimFLI *anim)
{
	memcpy(anim->vga_screen, &anim->fliPreload[anim->fliIndex], VGA_SIZE);
	anim->fliIndex+=VGA_SIZE;
}

void FliBlack(AnimFLI *anim)
{
	memset(anim->vga_screen, 0, VGA_SIZE);
	memset(anim->vga_pal, 0, 4*VGA_PAL_SIZE);
}

void DoType(AnimFLI *anim)
{
	const uint16 type = anim->CHKhdr.type;

	switch(type)
	{
		case FLI_COLOR:
			FliColor(anim);
			shouldUpdateFullFrame = true;
		break;

		case FLI_BRUN:
			FliBrun(anim);
			shouldUpdateFullFrame = true;
		break;

		case FLI_LC:
			FliLc(anim);
		break;

		case FLI_BLACK:
			FliBlack(anim);
			shouldUpdateFullFrame = true;
		break;

		case FLI_COPY:
			FliCopy(anim);
			shouldUpdateFullFrame = true;
		break;

//		case FLI_256_COLOR:
//		break;

//		case FLI_DELTA:
//		break;

//		case FLI_MINI:
//		break;

		default:
			//printf("%d\n",anim->CHKhdr.type);
		break;
	}
}


void FLIupdateFullFrame(uint16 *dst, uint32 *vga_pal, uint32 *vga32);
/*void FLIupdateFullFrame(uint16 *dst, uint32 *vga_pal, uint32 *vga32)
{
	int count = (VGA_WIDTH * VGA_HEIGHT) / 4;
	uint32 *dst32 = (uint32*)dst;
	do {
		const uint32 c = *vga32++;

		*dst32++ = vga_pal[(c >> 24) & 255] | (vga_pal[(c >> 16) & 255] >> PAL_PAD_BITS);
		*dst32++ = vga_pal[(c >> 8) & 255] | (vga_pal[c & 255] >> PAL_PAD_BITS);
	} while(--count > 0);
}*/

void FLIplayNextFrame(AnimFLI *anim)
{
	int i;

	shouldUpdateFullFrame = false;

	ReadFrameHDR(anim);
	anim->yline=0;

	for (i=0; i<anim->FRMhdr.chunks; i++)
	{
		ReadChunkHDR(anim);
		DoType(anim);
		anim->fliIndex = anim->nextchunk;
	}
	anim->fliIndex = anim->nextFrameIndex;

	if (anim->fliIndex >= anim->FLIhdr.size-sizeof(anim->FLIhdr)) {
		anim->nextFrameIndex = anim->fliIndex = anim->after_first_frame;
	}

	if (shouldUpdateFullFrame) {
		FLIupdateFullFrame(anim->bmp, anim->vga_pal, (uint32*)anim->vga_screen);
	}
}

void FLIload(AnimFLI *anim)
{
	char *filename = anim->filename;

	readBytesFromFileAndStore(filename, 0, sizeof(anim->FLIhdr), (char*)&anim->FLIhdr);

	anim->FLIhdr.size = LONG_ENDIAN_FLIP(anim->FLIhdr.size);
	anim->FLIhdr.width = SHORT_ENDIAN_FLIP(anim->FLIhdr.width);
	anim->FLIhdr.height = SHORT_ENDIAN_FLIP(anim->FLIhdr.height);

    anim->FLIhdr.magic = SHORT_ENDIAN_FLIP(anim->FLIhdr.magic);
    anim->FLIhdr.frames = SHORT_ENDIAN_FLIP(anim->FLIhdr.frames);

    anim->FLIhdr.depth = SHORT_ENDIAN_FLIP(anim->FLIhdr.depth);
    anim->FLIhdr.flags = SHORT_ENDIAN_FLIP(anim->FLIhdr.flags);
    anim->FLIhdr.speed = SHORT_ENDIAN_FLIP(anim->FLIhdr.speed);

    anim->FLIhdr.next = LONG_ENDIAN_FLIP(anim->FLIhdr.next);
    anim->FLIhdr.frit = LONG_ENDIAN_FLIP(anim->FLIhdr.frit);

	anim->fliPreload = AllocMem(anim->FLIhdr.size, MEMTYPE_ANY);

	readBytesFromFileAndStore(filename, sizeof(anim->FLIhdr), anim->FLIhdr.size - sizeof(anim->FLIhdr), anim->fliPreload);
}

AnimFLI *newAnimFLI(char *filename, uint16 *bmp)
{
	AnimFLI *anim = AllocMem(sizeof(AnimFLI), MEMTYPE_ANY);

	anim->filename = filename;
	anim->bmp = bmp;

	anim->fliIndex = 0;
	anim->nextFrameIndex = 0;
	anim->nextchunk = 0;
	anim->after_first_frame = 0;

	anim->vga_screen = (unsigned char*)AllocMem(VGA_SIZE, MEMTYPE_ANY);
	anim->vga_pal = (uint32*)AllocMem(4*VGA_PAL_SIZE, MEMTYPE_ANY);

	return anim;
}
