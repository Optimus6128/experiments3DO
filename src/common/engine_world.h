#ifndef ENGINE_WORLD_H
#define ENGINE_WORLD_H

#include "types.h"

#include "engine_main.h"
#include "mathutil.h"

typedef struct ObjectInfo
{
	int priority;
	bool visCheck;
}ObjectInfo;

typedef struct World
{
	Object3D **objects;
	ObjectInfo *objectInfo;
	BoundingBox *objectBbox;
	int nextObject;
	int maxObjects;

	Camera **cameras;
	int nextCamera;
	int maxCameras;

	Light **lights;
	int nextLight;
	int maxLights;

	int activeCamera;
}World;

World *initWorld(int maxObjects, int maxCameras, int maxLights);
void resetWorld(World *world, bool resetCameras, bool resetLights);

void addObjectToWorld(Object3D *object, int priority, bool visCheck, World *world);
void addCameraToWorld(Camera *camera, World *world);
void addLightToWorld(Light *light, World *world);

void resetWorldCameras(World *world);
void resetWorldLights(World *world);

void setActiveCamera(World *world, int camIndex);

void renderWorld(World *world);

#endif
