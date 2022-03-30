#ifndef ENGINE_SOFT_H
#define ENGINE_SOFT_H

#include "engine_main.h"
#include "engine_mesh.h"

#define COLOR_GRADIENTS_SHR 5
#define COLOR_ENVMAP_SHR COLOR_GRADIENTS_SHR
#define COLOR_GRADIENTS_SIZE (1 << COLOR_GRADIENTS_SHR)

enum {	RENDER_SOFT_METHOD_WIREFRAME, 
		RENDER_SOFT_METHOD_GOURAUD, 
		RENDER_SOFT_METHOD_ENVMAP, 
		RENDER_SOFT_METHOD_GOURAUD_ENVMAP, 
		RENDER_SOFT_METHOD_NUM };

void initEngineSoft(void);
void renderTransformedMeshSoft(Mesh *ms, ScreenElement *elements);

void setRenderSoftMethod(int method);
#endif
