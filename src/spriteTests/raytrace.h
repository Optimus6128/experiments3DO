#ifndef RAYTRACE_H
#define RAYTRACE_H

#define RT_WIDTH 128
#define RT_HEIGHT 128

#define RT_UPDATE_PIECES 16

#define COL_BITS 8
#define COL_BITS_OBJ 6

#define COLS_RANGE ((1<<COL_BITS)-1)
#define COLS_OBJ_RANGE ((1<<COL_BITS_OBJ)-1)

#include "core.h"

enum { OBJ_NOTHING, OBJ_PLANE, OBJ_SPHERE, OBJ_TYPES_COUNT };

void raytraceInit(void);
void raytraceRun(uint16 *buff, int updatePieceIndex, int ticks);

#endif
