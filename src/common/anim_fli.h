#ifndef ANIM_FLI_H
#define ANIM_FLI_H

#include "types.h"
#include "core.h"

#define FLI_COLOR       11
#define FLI_BLACK       13
#define FLI_COPY        16
#define FLI_LC          12
#define FLI_BRUN        15
#define FLI_DELTA       7
#define FLI_256_COLOR   4
#define FLI_MINI        18

#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)
#define VGA_PAL_SIZE 256
#define PAL_PAD_BITS 16

#define ORIG_FLI_HEADER_SIZE 128
#define ORIG_FRAME_HEADER_SIZE 16
#define ORIG_CHUNK_HEADER_SIZE 6

#define STREAM_BLOCK_SIZE 65536

typedef struct OrigFLIheader
{
    uint32 size;
	
    uint16 magic;
    uint16 frames;
	
    uint16 width;
    uint16 height;
	
    uint16 depth;
    uint16 flags;
	
    uint16 speed;
	// This was padded with an extra 16bit at this point from the compiler, so not sure if next and frit weren loaded correctly
	// Not sure, it works for now

    uint32 next;
    uint32 frit;
    // unsigned char expand[98];	// no need to store these bytes
	// Or maybe the padding was happening at this point.
} OrigFLIheader;

typedef struct FLIheader
{
    uint32 size;
    uint32 magic;	// was 16bit
    uint32 frames;	// was 16bit
    uint32 width;	// was 16bit
    uint32 height;	// was 16bit
    uint32 depth;	// was 16bit
    uint32 flags;	// was 16bit
    uint32 speed;	// was 16bit
    uint32 next;
    uint32 frit;
} FLIheader;

typedef struct FRAMEheader
{
    uint32 size;
    uint32 magic;	// was 16bit
    uint32 chunks;	// was 16bit
    // unsigned char expand[8];	// no need to store these bytes
} FRAMEheader;

typedef struct CHUNKheader
{
    uint32 size;
    uint32 type;	// was 16bit
} CHUNKheader;

typedef struct AnimFLI
{
	char *filename;
	uint16 *bmp;

	unsigned char *vga_screen;
	uint32 *vga_pal;	// padded 16bit 0x**00

	unsigned char *fliBuffer;
	uint32 dataIndex;
	uint32 fileIndex;

	uint32 nextFrameDataIndex;
	uint32 nextchunk;
	uint32 firstFrameSize;

	uint32 yline;
	bool streaming;

	FLIheader FLIhdr;
	FRAMEheader FRMhdr;
	CHUNKheader CHKhdr;

	Stream *fileStream;
} AnimFLI;

AnimFLI *newAnimFLI(char *filename, uint16 *bmp);
void FLIload(AnimFLI *anim, bool preLoad);

void FLIplayNextFrame(AnimFLI *anim);

#endif
