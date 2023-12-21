#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#include "mathutil.h"
#include "engine_view.h"
#include "engine_mesh.h"

#define USE_MAP_CEL_ASM

#define MAX_VERTEX_ELEMENTS_NUM 4096

#define PROJ_SHR 8

#define INSIDE				(1 << 0)
#define INSIDE_X 			(1 << 1)
#define INSIDE_Y 			(1 << 2)
#define OUTSIDE_LEFT		(1 << 3)
#define OUTSIDE_RIGHT		(1 << 4)
#define OUTSIDE_UP			(1 << 5)
#define OUTSIDE_DOWN		(1 << 6)
#define OUTSIDE_Z			(1 << 7)
#define OUTSIDE_ALL_BITS	((1 << 8) - 1)

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

void setGlobalLightDir(int vx, int vy, int vz);

void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *rotVecs);

void setScreenRegion(int posX, int posY, int width, int height);

#endif
