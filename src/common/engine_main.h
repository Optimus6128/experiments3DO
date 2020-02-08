#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#include "engine_mesh.h"

#define MAX_VERTICES_NUM 256

#define PROJ_SHR 8
#define REC_FPSHR 20
#define NUM_REC_Z 32768

void initEngine(void);

void rotateTranslateProjectVertices(mesh *ms);
void renderTransformedGeometry(mesh *ms);

void setScreenDimensions(int w, int h);

#endif
