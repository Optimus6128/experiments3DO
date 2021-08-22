#ifndef PROCGEN_MESH_H
#define PROCGEN_MESH_H

#include "engine_mesh.h"
#include "engine_texture.h"

enum {MESH_PLANE, MESH_CUBE, MESH_PYRAMID1, MESH_PYRAMID2, MESH_PYRAMID3, MESH_GRID};

Mesh *initGenMesh(int size, Texture *tex, int optionsFlags, int meshgenId, void *params);

#endif
