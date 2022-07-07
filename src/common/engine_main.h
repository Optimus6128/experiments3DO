#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#include "mathutil.h"
#include "engine_view.h"
#include "engine_mesh.h"

#define MAX_VERTEX_ELEMENTS_NUM 4096+256

#define PROJ_SHR 8

#define SHADE_TABLE_SHR 4
#define SHADE_TABLE_SIZE (1 << SHADE_TABLE_SHR)

extern int shadeTable[SHADE_TABLE_SIZE];


typedef struct ScreenElement
{
	int x,y,z;
	int c;
	int u,v;
	bool outside;
}ScreenElement;

typedef struct Object3D
{
	Mesh *mesh;
	Vector3D pos, rot;
	BoundingBox bbox;
}Object3D;

typedef struct Light
{
	Vector3D pos;
	Vector3D dir;
	bool isDirectional;
}Light;

void initEngine(bool usesSoftEngine);

Object3D* initObject3D(Mesh *ms);

void setObject3Dpos(Object3D *obj, int px, int py, int pz);
void setObject3Drot(Object3D *obj, int rx, int ry, int rz);
void setObject3Dmesh(Object3D *obj, Mesh *ms);

void renderObject3D(Object3D *obj, Camera *cam, Light **lights, int lightsNum);

Light *createLight(bool isDirectional);
void setLightPos(Light *light, int px, int py, int pz);
void setLightDir(Light *light, int vx, int vy, int vz);

void setGlobalLight(Light *light);

void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *rotVecs);

void setScreenRegion(int posX, int posY, int width, int height);

void useCPUtestPolygonOrder(bool enable);

#endif
