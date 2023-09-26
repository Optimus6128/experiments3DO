#include "anim_fli.h"
#include "file_utils.h"
#include "tools.h"

void FLIupdateFullFrame(uint16 *dst, uint32 *vga_pal, uint32 *vga32);

static bool shouldUpdateFullFrame;


static uint32 readU16backTo32(AnimFLI *anim)
{
	const uint32 dataIndex = anim->dataIndex;
	unsigned char *fliBuffer = anim->fliBuffer;

	const uint32 u0 = fliBuffer[dataIndex];
	const uint32 u1 = fliBuffer[dataIndex+1];
	const uint32 value = (u1 << 8) | u0;

	anim->dataIndex += 2;

	return value;
}

static uint32 readU32(AnimFLI *anim, bool advance)
{
	uint32 value;
	const uint32 dataIndex = anim->dataIndex;
	unsigned char *fliBuffer = anim->fliBuffer;

	if ((dataIndex & 3) == 0) {
		value = *((uint32*)&fliBuffer[dataIndex]);
		value = LONG_ENDIAN_FLIP(value);
	} else {
		const uint32 u0 = fliBuffer[dataIndex];
		const uint32 u1 = fliBuffer[dataIndex+1];
		const uint32 u2 = fliBuffer[dataIndex+2];
		const uint32 u3 = fliBuffer[dataIndex+3];

		value = (u3 << 24) | (u2 << 16) | (u1 << 8) | u0;
	}
	if (advance) anim->dataIndex += 4;

	return value;
}

void FliColor(AnimFLI *anim)
{
	int i, j, ci=0;
	unsigned char *fliBuffer = anim->fliBuffer;
	uint32 *vga_pal = anim->vga_pal;

	const int packets = readU16backTo32(anim);

	for (i=0; i<packets; i++)
	{
		const int colors2skip = fliBuffer[anim->dataIndex++];
		int colors2chng = fliBuffer[anim->dataIndex++];

		if (colors2chng==0) colors2chng = VGA_PAL_SIZE;
		ci+=colors2skip;
		for (j=0; j<colors2chng; j++)
		{
			const int r = (fliBuffer[anim->dataIndex++]>>1);
			const int g = (fliBuffer[anim->dataIndex++]>>1);
			const int b = (fliBuffer[anim->dataIndex++]>>1);
			vga_pal[ci++] = (r<<(10+PAL_PAD_BITS)) | (g<<(5+PAL_PAD_BITS)) | (b << PAL_PAD_BITS);
		}
	}
}


void FliBrun(AnimFLI *anim)
{
	int i, j, y, vi=0;
	unsigned char *fliBuffer = anim->fliBuffer;
	unsigned char *vga_screen = anim->vga_screen;

	anim->firstFrameSize = anim->FRMhdr.size;

	for (y=0; y<anim->FLIhdr.height; y++) {
		const unsigned char packets = fliBuffer[anim->dataIndex++];
		for (i=0; i<packets; i++) {
			int8 size_count = (int8)fliBuffer[anim->dataIndex++];
			const unsigned char data = fliBuffer[anim->dataIndex++];

			if (size_count>=0) {
				for (j=0; j<size_count; j++) {
					vga_screen[vi++] = data;
				}
			} else {
				size_count = -size_count;

				vga_screen[vi++] = data;
				for (j=1; j<size_count; j++) {
					vga_screen[vi++] = fliBuffer[anim->dataIndex++];
				}
			}
		}
	}
}

void FliLc(AnimFLI *anim)
{
	int i, j, n, vi=0;

	uint16 *dst = anim->bmp;
	unsigned char *fliBuffer = anim->fliBuffer;
	unsigned char *vga_screen = anim->vga_screen;
	uint32 *vga_pal = anim->vga_pal;

	const uint32 lines_skip = readU16backTo32(anim);
	const uint32 lines_chng = readU16backTo32(anim);

	anim->yline += lines_skip;
	vi = anim->yline * VGA_WIDTH;

	for (i=0; i<lines_chng; i++) {
		const unsigned char packets = fliBuffer[anim->dataIndex++];

		for (n=0; n<packets; n++) {
			const unsigned char skip_count = fliBuffer[anim->dataIndex++];
			int8 size_count = fliBuffer[anim->dataIndex++];

			vi+=skip_count;
			if (size_count<0) {
				const unsigned char data = fliBuffer[anim->dataIndex++];
				const uint16 col = vga_pal[data] >> PAL_PAD_BITS;
				size_count = -size_count;
				for (j=0; j<size_count; j++) {
					vga_screen[vi] = data;
					dst[vi] = col;
					vi++;
				}
			} else {
				for (j=0; j<size_count; j++) {
					const unsigned char data = fliBuffer[anim->dataIndex++];
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
	memcpy(anim->vga_screen, &anim->fliBuffer[anim->dataIndex], VGA_SIZE);
	anim->dataIndex+=VGA_SIZE;
}

void FliBlack(AnimFLI *anim)
{
	memset(anim->vga_screen, 0, VGA_SIZE);
	memset(anim->vga_pal, 0, 4*VGA_PAL_SIZE);
}

void DoType(AnimFLI *anim)
{
	const uint32 type = anim->CHKhdr.type;

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


void ReadFrameHDR(AnimFLI *anim)
{
	anim->FRMhdr.size = readU32(anim, true);

	anim->FRMhdr.magic = readU16backTo32(anim);
	anim->FRMhdr.chunks = readU16backTo32(anim);

	anim->dataIndex += 8;
}

void ReadChunkHDR(AnimFLI *anim)
{
	anim->CHKhdr.size = readU32(anim, true);
	anim->nextchunk = anim->dataIndex - 4 + anim->CHKhdr.size;
	anim->CHKhdr.type = readU16backTo32(anim);
}

static void streamNextBlock(AnimFLI *anim, int size)
{
	anim->dataIndex = anim->nextFrameDataIndex = 0;
	readBytesFromFileStream(anim->fileIndex, size, anim->fliBuffer, anim->fileStream);
}

/*static void streamNextBlockIfNeeded(AnimFLI *anim)
{
	const int frameSize = readU32(anim, false);

	if (anim->dataIndex + frameSize > STREAM_BLOCK_SIZE) {
		streamNextBlock(anim, STREAM_BLOCK_SIZE);
	}
}*/

void FLIplayNextFrame(AnimFLI *anim)
{
	int i;

	// Check if need to stream next block and read Frame Header
	//streamNextBlockIfNeeded(anim);

	// Or alternative test, stream each frame individually
	if (anim->streaming) streamNextBlock(anim, readU32(anim, false) + 4);

	ReadFrameHDR(anim);
	anim->nextFrameDataIndex += anim->FRMhdr.size;


	// Main chunk decoding loop
	anim->yline=0;
	shouldUpdateFullFrame = false;
	for (i=0; i<anim->FRMhdr.chunks; i++)
	{
		ReadChunkHDR(anim);
		DoType(anim);
		anim->dataIndex = anim->nextchunk;
	}
	anim->fileIndex += anim->FRMhdr.size;
	anim->dataIndex = anim->nextFrameDataIndex;


	// Check if at end of animation and reset
	if (anim->fileIndex >= anim->FLIhdr.size) {
		anim->fileIndex = ORIG_FLI_HEADER_SIZE + anim->firstFrameSize;
		if (anim->streaming) {
			streamNextBlock(anim, STREAM_BLOCK_SIZE);
		} else {
			anim->dataIndex = anim->nextFrameDataIndex = anim->firstFrameSize;
		}
	}

	// Make a full frame copy from vga buffer to 16bpp CEL if necessary
	if (shouldUpdateFullFrame) {
		FLIupdateFullFrame(anim->bmp, anim->vga_pal, (uint32*)anim->vga_screen);
	}
}

void FLIload(AnimFLI *anim, bool preLoad)
{
	OrigFLIheader origFliHdr;
	int loadSize = STREAM_BLOCK_SIZE;

	anim->fileStream = openFileStream(anim->filename);

	readBytesFromFileStream(0, sizeof(OrigFLIheader), (char*)&origFliHdr, anim->fileStream);
	anim->fileIndex = ORIG_FLI_HEADER_SIZE;

	anim->FLIhdr.size = LONG_ENDIAN_FLIP(origFliHdr.size);
	anim->FLIhdr.width = SHORT_ENDIAN_FLIP(origFliHdr.width);
	anim->FLIhdr.height = SHORT_ENDIAN_FLIP(origFliHdr.height);

    anim->FLIhdr.magic = SHORT_ENDIAN_FLIP(origFliHdr.magic);
    anim->FLIhdr.frames = SHORT_ENDIAN_FLIP(origFliHdr.frames);

    anim->FLIhdr.depth = SHORT_ENDIAN_FLIP(origFliHdr.depth);
    anim->FLIhdr.flags = SHORT_ENDIAN_FLIP(origFliHdr.flags);
    anim->FLIhdr.speed = SHORT_ENDIAN_FLIP(origFliHdr.speed);

    anim->FLIhdr.next = LONG_ENDIAN_FLIP(origFliHdr.next);
    anim->FLIhdr.frit = LONG_ENDIAN_FLIP(origFliHdr.frit);

	if (preLoad) {
		loadSize = anim->FLIhdr.size - ORIG_FLI_HEADER_SIZE;
	}
	anim->fliBuffer = AllocMem(loadSize, MEMTYPE_ANY);
	readBytesFromFileStream(ORIG_FLI_HEADER_SIZE, loadSize, anim->fliBuffer, anim->fileStream);

	anim->streaming = !preLoad;

	//again crashes, wtf
	if (preLoad) closeFileStream(anim->fileStream);
}

AnimFLI *newAnimFLI(char *filename, uint16 *bmp)
{
	AnimFLI *anim = AllocMem(sizeof(AnimFLI), MEMTYPE_ANY);

	anim->filename = filename;
	anim->bmp = bmp;

	anim->dataIndex = anim->nextFrameDataIndex = 0;
	anim->nextchunk = 0;

	anim->vga_screen = (unsigned char*)AllocMem(VGA_SIZE, MEMTYPE_ANY);
	anim->vga_pal = (uint32*)AllocMem(4*VGA_PAL_SIZE, MEMTYPE_ANY);

	return anim;
}
