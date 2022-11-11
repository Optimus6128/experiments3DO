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

typedef struct FLIheader
{
    uint32 size;
    uint16 magic;
    uint16 frames;
    uint16 width;
    uint16 height;
    uint16 depth;
    uint16 flags;
    uint16 speed;
    uint32 next;
    uint32 frit;
    unsigned char expand[98];	// was 102 but because of word aligment it brought size to 132 and broke the offset of 128 (crazy bug that lost my time)
} FLIheader;

typedef struct FRAMEheader
{
    uint32 size;
    uint16 magic;
    uint16 chunks;
    unsigned char expand[8];
} FRAMEheader;

typedef struct CHUNKheader
{
    uint32 size;
    uint16 type;
} CHUNKheader;

typedef struct AnimFLI
{
	char *filename;
	uint16 *bmp;

	unsigned char *vga_screen;
	uint16 *vga_pal;

	unsigned char *fliPreload;
	uint32 fliIndex;

	uint32 nextFrameIndex;
	uint32 nextchunk;
	uint32 after_first_frame;

	uint32 yline;

	FLIheader FLIhdr;
	FRAMEheader FRMhdr;
	CHUNKheader CHKhdr;

} AnimFLI;

AnimFLI *newAnimFLI(char *filename, uint16 *bmp);
void FLIload(AnimFLI *anim);

void FLIplayNextFrame(AnimFLI *anim);

#endif
