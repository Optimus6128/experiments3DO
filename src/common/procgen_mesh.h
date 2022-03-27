#ifndef PROCGEN_MESH_H
#define PROCGEN_MESH_H

#include "engine_mesh.h"
#include "engine_texture.h"

typedef struct MeshgenParams
{
	int size;
	int divisions;

	Point2D *procPoints;
	int numProcPoints;

	bool capTop, capBottom;
} MeshgenParams;

#define DEFAULT_MESHGEN_PARAMS(size) makeDefaultMeshgenParams(size)

enum {MESH_PLANE, MESH_CUBE, MESH_CUBE_TRI, MESH_ROMBUS, MESH_PYRAMID1, MESH_PYRAMID2, MESH_PYRAMID3, MESH_GRID, MESH_SQUARE_COLUMNOID, MESH_VOLUME_SLICES };

Mesh *initGenMesh(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex);

MeshgenParams makeDefaultMeshgenParams(int size);
MeshgenParams makeMeshgenGridParams(int size, int divisions);
MeshgenParams makeMeshgenSquareColumnoidParams(int size, Point2D *points, int numPoints, bool capTop, bool capBottom);

#endif
