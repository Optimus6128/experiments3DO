#ifndef CORE_H
#define CORE_H

#include "types.h"

// Hack for the lame define of uchar as char (broke some code where sign/unsigned mattered)
// I assume the first compiled files will include core.h which is true for these experiments. All the effect_ files.
// But this is not perfect. Depends on the order of compilation.
// Alternatively I could either replace all uchar/ubyte/uint8 in the code with unsigned char and remember to only use that
// OR define some U8 type and remember to use that. But I don't like that I have to do this.
//#define uchar uchar_
//#define ubyte ubyte_
//#define uint8 uint8_
//typedef unsigned char uchar_;
//typedef unsigned char ubyte_;
//typedef unsigned char uint8_;

// UPDATE! I keep the old solution but I comment out because I think it's dangerous.
// What if some 3DO libary functions expect uchar to be signed char?
// Best solution unfortunatelly, in all my code, I'll replace uint8/ubyte/uchar when I use it, with unsigned char and remember to not use these types.
// And keep uint8 where it makes sense, for example in the fileutils I pass a uint8 somewhere that feeds into the API FILE functions that expects that. The defines above would give me an error of type missmatch.
// So, use "unsigned char" when you really want unsigned char, and avoid the defined types for this one. But you have to remember, that's the problem! You could sneak in something wrong oneday.
// Also I cannot avoid including types.h and include mine, because include other 3DO API headers will include types already.

// I considered a 3rd solution I won't write here, but thinking again this will change types passed to the API functions, so in your code never use uint8/ubyte/uchar, just type unsigned char.

// I might delete this rant..


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

#include "cel_helpers.h"

#define CORE_EMPTY					0
#define CORE_SHOW_FPS				(1 << 1)
#define CORE_SHOW_MEM				(1 << 2)
#define CORE_SHOW_BUFFERS			(1 << 3)
#define CORE_DEFAULT_INPUT			(1 << 4)
#define CORE_MENU					(1 << 5)
#define CORE_NO_VSYNC				(1 << 6)
#define CORE_NO_CLEAR_FRAME			(1 << 7)

// 8-15 -> vram_buffers, back_buffers and antialias bits

#define CORE_INIT_3D_ENGINE			(1 << 16)
#define CORE_INIT_3D_ENGINE_SOFT	(1 << 17)

#define CORE_DEBUG					(1 << 31)


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
#define CORE_VRAM_MAXBUFFERS			(6 << VRAM_NUMBUFFERS_BITS_START)
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

void updateLoadingBar(int bar, int status, int max);

#endif
