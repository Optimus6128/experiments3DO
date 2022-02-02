#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"

#define MESH_OPTION_FAST_MAPCEL		(1 << 0)
#define MESH_OPTION_CPU_CCW_TEST	(1 << 1)
#define MESH_OPTIONS_DEFAULT (MESH_OPTION_FAST_MAPCEL | MESH_OPTION_CPU_CCW_TEST)

enum {
	MESH_RENDER_SOFT = 1,
	MESH_RENDER_HARD = 2	// Can have (MESH_RENDER_SOFT | MESH_RENDER_HARD) for both
};

typedef struct QuadData
{
	ubyte textureId;
	ubyte palId;
}QuadData;

typedef struct TexCoords
{
	int u,v;
}TexCoords;

typedef struct Mesh
{
	Vertex *vrtx;
	int vrtxNum;

	int *index;
	int indexNum;
	
	QuadData *quad;
	int quadsNum;

	CCB *cel;
	uint32 *indexCol;
	TexCoords *indexTC;

	Texture *tex;
	ubyte texturesNum;

	int renderType;
}Mesh;


Mesh* initMesh(int vrtxNum, int quadsNum, int renderType);
// TODO: Mesh *loadMesh(char *path);

void prepareCelList(Mesh *ms);

//void setMeshPosition(Mesh *ms, int px, int py, int pz);
//void setMeshRotation(Mesh *ms, int rx, int ry, int rz);
void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw);
void setMeshTransparency(Mesh *ms, bool enable);
void setMeshTranslucency(Mesh *ms, bool enable);
void setMeshDottedDisplay(Mesh *ms, bool enable);

void updateMeshCELs(Mesh *ms);

#endif
