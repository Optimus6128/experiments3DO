#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"
#include "core.h"

#define MESH_OPTION_RENDER_SOFT8	(1 << 0)
#define MESH_OPTION_RENDER_SOFT16	(1 << 1)
#define MESH_OPTION_RENDER_SOFT		(MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_RENDER_SOFT16)
#define MESH_OPTION_ENABLE_LIGHTING	(1 << 2)
#define MESH_OPTION_ENABLE_ENVMAP	(1 << 3)
#define MESH_OPTION_FAST_MAPCEL		(1 << 4)
#define MESH_OPTION_CPU_POLYTEST	(1 << 5)
#define MESH_OPTIONS_DEFAULT 		(MESH_OPTION_FAST_MAPCEL | MESH_OPTION_CPU_POLYTEST)


typedef struct PolyData
{
	ubyte numPoints;
	ubyte textureId;
	ubyte palId;
	ubyte texShifts;	// Width/Height bits, WWWWHHHH
}PolyData;

typedef struct TexCoords
{
	int u,v;
}TexCoords;

typedef struct Mesh
{
	Vertex *vertex;
	int verticesNum;

	int *index;
	int indicesNum;

	PolyData *poly;
	int polysNum;

	Vector3D *polyNormal;
	Vector3D *vertexNormal;

	int *lineIndex;
	int linesNum;

	CCB *cel;
	int *vertexCol;
	TexCoords *vertexTC;

	Texture *tex;
	int texturesNum;

	int renderType;
}Mesh;


Mesh* initMesh(int verticesNum, int polysNum, int indicesNum, int linesNum, int renderType);
// TODO: Mesh *loadMesh(char *path);

void prepareCelList(Mesh *ms);

void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw);
void setMeshTransparency(Mesh *ms, bool enable);
void setMeshTranslucency(Mesh *ms, bool enable);
void setMeshDottedDisplay(Mesh *ms, bool enable);

void updateMeshCELs(Mesh *ms);

#endif
