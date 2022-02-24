#ifndef ENGINE_SOFT_H
#define ENGINE_SOFT_H

#include "engine_mesh.h"

#define COLOR_GRADIENTS_SHR 5
#define COLOR_GRADIENTS_SIZE (1 << COLOR_GRADIENTS_SHR)

void initEngineSoft(void);
void renderTransformedMeshSoft(Mesh *ms, Vertex *vertices);

#endif
