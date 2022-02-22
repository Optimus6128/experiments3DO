#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#include "engine_mesh.h"

#define MAX_VERTICES_NUM 1024
#define MAX_NORMALS_NUM 1024

#define PROJ_SHR 8
#define REC_FPSHR 20
#define NUM_REC_Z 32768

#define NORMAL_SHIFT 8

typedef struct Object3D
{
	Mesh *mesh;

	int posX, posY, posZ;
	int rotX, rotY, rotZ;
}Object3D;

void initEngine(void);

Object3D* initObject3D(Mesh *ms);

void setObject3Dpos(Object3D *obj, int px, int py, int pz);
void setObject3Drot(Object3D *obj, int rx, int ry, int rz);

void renderObject3D(Object3D *obj);
void renderObject3Dsoft(Object3D *obj);

void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *rotVecs);	// I will hide this again after I fix the volume

void setScreenRegion(int posX, int posY, int width, int height);

void useCPUtestPolygonOrder(bool enable);

#endif
