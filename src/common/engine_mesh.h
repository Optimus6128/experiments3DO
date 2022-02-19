#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"

#define MESH_OPTION_RENDER_SOFT		(1 << 0)
#define MESH_OPTION_RENDER_HARD		(1 << 1)
#define MESH_OPTIONS_DEFAULT MESH_OPTION_RENDER_HARD

typedef struct PolyData
{
	int textureId;
	int palId;
}PolyData;

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

	PolyData *poly;
	int quadsNum;
	int trianglesNum;
	// indices should be stored in a way that first come the quads and then the triangles

	CCB *cel;
	uint32 *indexCol;
	TexCoords *indexTC;

	Texture *tex;
	int texturesNum;

	int renderType;
}Mesh;


Mesh* initMesh(int vrtxNum, int quadsNum, int trianglesNum, int renderType);
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
