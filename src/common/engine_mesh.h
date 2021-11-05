#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"

#define MESH_OPTION_FAST_MAPCEL		(1 << 0)
#define MESH_OPTION_CPU_CCW_TEST	(1 << 1)
#define MESH_OPTIONS_DEFAULT (MESH_OPTION_FAST_MAPCEL | MESH_OPTION_CPU_CCW_TEST)

typedef struct QuadData
{
	CCB *cel;
	ubyte textureId;
	ubyte palId;
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
	ubyte texturesNum;

	int posX, posY, posZ;
	int rotX, rotY, rotZ;

	bool useFastMapCel;
	bool useCPUccwTest;
}Mesh;


Mesh* initMesh(int vrtxNum, int quadsNum);
// TODO: Mesh *loadMesh(char *path);

void prepareCelList(Mesh *ms);

void setMeshPosition(Mesh *ms, int px, int py, int pz);
void setMeshRotation(Mesh *ms, int rx, int ry, int rz);
void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw);
void setMeshTransparency(Mesh *ms, bool enable);
void setMeshTranslucency(Mesh *ms, bool enable);
void setMeshDottedDisplay(Mesh *ms, bool enable);

void updateMeshCELs(Mesh *ms);

#endif
