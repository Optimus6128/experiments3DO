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

enum {MESH_PLANE, MESH_CUBE, MESH_CUBE_TRI, MESH_ROMBUS, MESH_PRISM, MESH_PYRAMID1, MESH_PYRAMID2, MESH_PYRAMID3, MESH_GRID, MESH_SQUARE_COLUMNOID, MESH_STARS, MESH_SKYBOX, MESH_PARTICLES, MESH_VOLUME_SLICES };

Mesh *initGenMesh(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex);
Mesh *subdivMesh(Mesh *srcMesh);

MeshgenParams makeDefaultMeshgenParams(int size);
MeshgenParams makeMeshgenGridParams(int size, int divisions);
MeshgenParams makeMeshgenSkyboxParams(int size, int subdivisions);
MeshgenParams makeMeshgenSquareColumnoidParams(int size, Point2D *points, int numPoints, bool capTop, bool capBottom);
MeshgenParams makeMeshgenStarsParams(int distance, int numStars);
MeshgenParams makeMeshgenParticlesParams(int numParticles);

void calculateMeshNormals(Mesh *ms);

#endif
