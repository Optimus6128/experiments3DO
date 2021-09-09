#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#include "engine_mesh.h"

#define MAX_VERTICES_NUM 1024

#define PROJ_SHR 8
#define REC_FPSHR 20
#define NUM_REC_Z 32768

void initEngine(void);

void transformGeometry(Mesh *ms);
void renderTransformedGeometry(Mesh *ms);

void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *rotVecs);

void setScreenDimensions(int w, int h);

#endif
