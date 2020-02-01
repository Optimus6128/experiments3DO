#ifndef CORE_H
#define CORE_H

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
#define CORE_DEBUG			1 << 0
#define CORE_SHOW_FPS		1 << 1
#define CORE_SHOW_MEM		1 << 2
#define CORE_DEFAULT_INPUT	1 << 3
#define CORE_MENU			1 << 4
#define CORE_DEFAULT (CORE_SHOW_FPS | CORE_DEFAULT_INPUT | CORE_MENU)
#define CORE_SIMPLE (CORE_SHOW_FPS)


void coreInit(void(*initFunc)(), int flags);
void coreRun(void(*mainLoopFunc)());

#endif
