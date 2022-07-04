#include "engine_world.h"

void setActiveCamera(World *world, int camIndex)
{
	if (camIndex > 0 && camIndex < world->maxCameras) {
		world->activeCamera = camIndex;
	}
}

void resetWorldCameras(World *world)
{
	int i;
	for (i=0; i<world->maxCameras; ++i) {
		world->cameras[i] = NULL;
	}
	world->nextCamera = 0;
}

void resetWorldLights(World *world)
{
	int i;
	for (i=0; i<world->maxLights; ++i) {
		world->lights[i] = NULL;
	}
	world->nextLight = 0;
}

void resetWorld(World *world, bool resetCameras, bool resetLights)
{
	int i;
	for (i=0; i<world->maxObjects; ++i) {
		world->objects[i] = NULL;
	}
	world->nextObject = 0;

	if (resetCameras) {
		resetWorldCameras(world);
	}
	if (resetLights) {
		resetWorldLights(world);
	}

	setActiveCamera(world, 0);
}

World *initWorld(int maxObjects, int maxCameras, int maxLights)
{
	World *world = (World*)AllocMem(sizeof(World), MEMTYPE_ANY);

	if (maxObjects <= 0) maxObjects = 1;
	if (maxCameras <= 0) maxCameras = 1;
	if (maxLights <= 0) maxLights = 1;

	world->objects = (Object3D**)AllocMem(maxObjects * sizeof(Object3D*), MEMTYPE_ANY);
	world->maxObjects = maxObjects;

	world->cameras = (Camera**)AllocMem(maxCameras * sizeof(Camera*), MEMTYPE_ANY);
	world->maxCameras = maxCameras;

	world->lights = (Light**)AllocMem(maxLights * sizeof(Light*), MEMTYPE_ANY);
	world->maxLights = maxLights;

	resetWorld(world, true, true);

	return world;
}

void addObjectToWorld(Object3D *object, World *world)
{
	if (world->nextObject < world->maxObjects) {
		world->objects[world->nextObject++] = object;
	}
}

void addCameraToWorld(Camera *camera, World *world)
{
	if (world->nextCamera < world->maxCameras) {
		world->cameras[world->nextCamera++] = camera;
	}
}

void addLightToWorld(Light *light, World *world)
{
	if (world->nextLight < world->maxLights) {
		world->lights[world->nextLight++] = light;
	}
}

void renderWorld(World *world)
{
	int i;
	Camera *camera = world->cameras[world->activeCamera];

	for (i=0; i<world->nextObject; ++i) {
		renderObject3D(world->objects[i], camera, NULL, 0);
	}
}
