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
    uint8 expand[102];
} FLIheader;

typedef struct FRAMEheader
{
    uint32 size;
    uint16 magic;
    uint16 chunks;
    uint8 expand[8];
} FRAMEheader;

typedef struct CHUNKheader
{
    uint32 size;
    uint16 type;
} CHUNKheader;

typedef struct AnimFLI
{
	char *filename;
} AnimFLI;

AnimFLI *newAnimFLI(char *filename);
void FLIload(AnimFLI *anim);

void FLIshow(AnimFLI *anim, uint16 *dst);
void FLIplayNextFrame(AnimFLI *anim);

#endif
