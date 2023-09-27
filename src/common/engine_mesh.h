#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"
#include "core.h"

#define MESH_OPTION_RENDER_POINTS		(1 << 0)
#define MESH_OPTION_RENDER_BILLBOARDS	(1 << 1)
#define MESH_OPTION_RENDER_SOFT8		(1 << 2)
#define MESH_OPTION_RENDER_SOFT16		(1 << 3)
#define MESH_OPTION_RENDER_SOFT			(MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_RENDER_SOFT16)
#define MESH_OPTION_ENABLE_LIGHTING		(1 << 4)
#define MESH_OPTION_ENABLE_ENVMAP		(1 << 5)
#define MESH_OPTION_FAST_MAPCEL			(1 << 6)
#define MESH_OPTION_CPU_POLYTEST		(1 << 7)
#define MESH_OPTION_NO_POLYCLIP			(1 << 8)
#define MESH_OPTION_NO_POLYSORT			(1 << 9)
#define MESH_OPTION_NO_TRANSLATE		(1 << 10)
#define MESH_OPTIONS_DEFAULT 			(MESH_OPTION_FAST_MAPCEL | MESH_OPTION_CPU_POLYTEST)

#define MESH_LOAD_SKIP_LINES		(1 << 0)
#define MESH_LOAD_FLIP_POLYORDER	(1 << 1)

typedef struct PolyData
{
	unsigned char numPoints;
	unsigned char textureId;
	unsigned char palId;
	unsigned char texShifts;	// Width/Height bits, WWWWHHHH

	unsigned short offsetU;
	unsigned short offsetV;
	unsigned short subtexWidth;
	unsigned short subtexHeight;
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

	int renderType;
}Mesh;

typedef struct ElementsSize
{
	int verticesNum;
	int polysNum;
	int indicesNum;
	int linesNum;
} ElementsSize;


ElementsSize *getElementsSize(int verticesNum, int polysNum, int indicesNum, int linesNum);

Mesh *initMesh(ElementsSize *elSize, int renderType, Texture *tex);
Mesh *loadMesh(char *path, int loadOptions, int meshOptions, Texture *tex);
void destroyMesh(Mesh *ms);

void prepareCelList(Mesh *ms);

void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw);
void setMeshTransparency(Mesh *ms, bool enable);
void setMeshTranslucency(Mesh *ms, bool enable, bool additive);
void setMeshDottedDisplay(Mesh *ms, bool enable);

void setMeshPolygonCPUbackfaceTest(Mesh *ms, bool enable);

void setMeshTexture(Mesh *ms, Texture *tex);
void setMeshPaletteIndex(Mesh *ms, int palIndex);

void setAllPolyData(Mesh *ms, int numPoints, int textureId, int palId);
void updatePolyTexData(Mesh *ms);
void flipMeshPolyOrder(Mesh *ms);
void scaleMesh(Mesh *ms, int scaleX, int scaleY, int scaleZ);
void flipMeshVerticesIfNeg(Mesh *ms, bool flipX, int flipY, bool flipZ);

void updateMeshCELs(Mesh *ms);

#endif
