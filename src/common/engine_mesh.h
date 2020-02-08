#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "engine_texture.h"
#include "mathutil.h"

typedef struct quadData
{
	texture *tex;
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

    int posX, posY, posZ;
    int rotX, rotY, rotZ;
}mesh;


enum {MESH_PLANE, MESH_CUBE, MESH_GRID};

mesh *initMesh(int type, int size, int divisions, int textureId);

void setMeshPosition(mesh *ms, int px, int py, int pz);
void setMeshRotation(mesh *ms, int rx, int ry, int rz);

#endif
