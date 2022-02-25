#ifndef CORE_H
#define CORE_H

#include "types.h"

#include "displayutils.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "operamath.h"
#include "math.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "event.h"
#include "controlpad.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "operror.h"
#include "directory.h"
#include "directoryfunctions.h"

#include "stdio.h"
#include "graphics.h"
#include "3dlib.h"
#include "Init3DO.h"
#include "Form3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "3d_examples.h"
#include "getvideoinfo.h"


#define CORE_EMPTY			0
#define CORE_SHOW_FPS		(1 << 1)
#define CORE_SHOW_MEM		(1 << 2)
#define CORE_SHOW_BUFFERS	(1 << 3)
#define CORE_DEFAULT_INPUT	(1 << 4)
#define CORE_MENU			(1 << 5)
#define CORE_NO_VSYNC		(1 << 6)
#define CORE_NO_CLEAR_FRAME		(1 << 7)
#define CORE_DEBUG			(1 << 31)


#define VRAM_NUMBUFFERS_BITS_START 8
#define MAX_NUMBUFFER_BITS 3
#define OFFSCREEN_NUMBUFFERS_BITS_START (VRAM_NUMBUFFERS_BITS_START + MAX_NUMBUFFER_BITS)
#define OTHER_GRAPHICS_OPTIONS_BITS_START (OFFSCREEN_NUMBUFFERS_BITS_START + MAX_NUMBUFFER_BITS)

// Omit these flag bits and a default will be set (2 screen buffers, 1 offscreen buffer)
// I will allow 3bits for vram buffers (0 to 7) and another 3bits for offscreen buffers
// After 6 total buffers of 320*240*16bpp you might have already filled the 1MB VRAM anyway
#define CORE_VRAM_SINGLEBUFFER			(1 << VRAM_NUMBUFFERS_BITS_START)
#define CORE_VRAM_DOUBLEBUFFER			(2 << VRAM_NUMBUFFERS_BITS_START)
#define CORE_VRAM_BUFFERS(N)			(N << VRAM_NUMBUFFERS_BITS_START)
#define CORE_OFFSCREEN_BUFFERS(N)		(N << OFFSCREEN_NUMBUFFERS_BITS_START)

#define CORE_HORIZONTAL_ANTIALIASING	(1 << OTHER_GRAPHICS_OPTIONS_BITS_START)
#define CORE_VERTICAL_ANTIALIASING		(1 << (OTHER_GRAPHICS_OPTIONS_BITS_START + 1))


#define CORE_DEFAULT	(CORE_SHOW_FPS | CORE_DEFAULT_INPUT | CORE_MENU)
#define CORE_SIMPLE		(CORE_SHOW_FPS)


void coreInit(void(*initFunc)(), uint32 flags);
void coreRun(void(*mainLoopFunc)());

void setShowFps(bool on);
void setShowMem(bool on);
void setShowBuffers(bool on);

void vramCpy(void* bufferSrc, void* bufferDst, int length);
void vramSet(uint32 c, void* bufferDst, int length);

#endif
