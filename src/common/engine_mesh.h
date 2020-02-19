#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"

#define MESH_OPTION_FAST_MAPCEL		(1 << 0)
#define MESH_OPTION_CPU_CCW_TEST	(1 << 1)
#define MESH_OPTIONS_DEFAULT (MESH_OPTION_FAST_MAPCEL | MESH_OPTION_CPU_CCW_TEST)

typedef struct QuadData
{
	int textureId;
	CCB *cel;
}QuadData;

typedef struct Mesh
{
	Vertex *vrtx;
	int vrtxNum;

	int *index;
	int indexNum;

	QuadData *quad;
	int quadsNum;

	Texture *tex;
	int texturesNum;

	int posX, posY, posZ;
	int rotX, rotY, rotZ;

	bool useFastMapCel;
	bool useCPUccwTest;
}Mesh;


enum {MESH_PLANE, MESH_CUBE, MESH_GRID};

Mesh *initMesh(int type, int size, int divisions, Texture *tex, int optionsFlags);

void setMeshPosition(Mesh *ms, int px, int py, int pz);
void setMeshRotation(Mesh *ms, int rx, int ry, int rz);
void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw);
void setMeshTranslucency(Mesh *ms, bool enable);

#endif
