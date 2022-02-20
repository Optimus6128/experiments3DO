#ifndef PROCGEN_MESH_H
#define PROCGEN_MESH_H

#include "engine_mesh.h"
#include "engine_texture.h"

typedef struct MeshgenParams
{
	int size;
	int divisions;
} MeshgenParams;

#define DEFAULT_MESHGEN_PARAMS(size) makeDefaultMeshgenParams(size)

enum {MESH_PLANE, MESH_CUBE, MESH_CUBE_TRI, MESH_PYRAMID1, MESH_PYRAMID2, MESH_PYRAMID3, MESH_GRID};

Mesh *initGenMesh(int meshgenId, int optionsFlags, const MeshgenParams params, Texture *tex);

MeshgenParams makeDefaultMeshgenParams(int size);
MeshgenParams makeMeshgenGridParams(int size, int divisions);

#endif
