#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#include "engine_mesh.h"

#define MAX_VERTEX_ELEMENTS_NUM 2048

#define PROJ_SHR 8

#define SHADE_TABLE_SHR 4
#define SHADE_TABLE_SIZE (1 << SHADE_TABLE_SHR)

extern int shadeTable[SHADE_TABLE_SIZE];


typedef struct ScreenElement
{
	int x,y,z;
	int c;
	int u,v;
}ScreenElement;

typedef struct Object3D
{
	Mesh *mesh;

	int posX, posY, posZ;
	int rotX, rotY, rotZ;
}Object3D;

void initEngine(bool usesSoftEngine);

Object3D* initObject3D(Mesh *ms);

void setObject3Dpos(Object3D *obj, int px, int py, int pz);
void setObject3Drot(Object3D *obj, int rx, int ry, int rz);
void setObject3Dmesh(Object3D *obj, Mesh *ms);

void renderObject3D(Object3D *obj);
void renderObject3Dsoft(Object3D *obj);

void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *rotVecs);	// I will hide this again after I fix the volume

void setScreenRegion(int posX, int posY, int width, int height);

void useCPUtestPolygonOrder(bool enable);

#endif
