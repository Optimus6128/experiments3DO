#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"

#define MESH_OPTION_FAST_MAPCEL		(1 << 0)
#define MESH_OPTION_CPU_CCW_TEST	(1 << 1)
#define MESH_OPTIONS_DEFAULT (MESH_OPTION_FAST_MAPCEL | MESH_OPTION_CPU_CCW_TEST)

typedef struct quadData
{
	int textureId;
	CCB *cel;
}quadData;

typedef struct mesh
{
    vertex *vrtx;
    int vrtxNum;

    int *index;
    int indexNum;

    quadData *quad;
    int quadsNum;

	texture *tex;
	int texturesNum;

    int posX, posY, posZ;
    int rotX, rotY, rotZ;

	bool useFastMapCel;
	bool useCPUccwTest;
}mesh;


enum {MESH_PLANE, MESH_CUBE, MESH_GRID};

mesh *initMesh(int type, int size, int divisions, texture *tex, int optionsFlags);

void setMeshPosition(mesh *ms, int px, int py, int pz);
void setMeshRotation(mesh *ms, int rx, int ry, int rz);
void setMeshPolygonOrder(mesh *ms, bool cw, bool ccw);
void setMeshTranslucency(mesh *ms, bool enable);

#endif
