#include "engine_world.h"

int sortedObjectIndex[256];
int sortedObjectsNum = 0;

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
		world->objectInfo[i].priority = 0;
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
	world->objectInfo = (ObjectInfo*)AllocMem(maxObjects * sizeof(ObjectInfo), MEMTYPE_ANY);
	world->objectBbox = (BoundingBox*)AllocMem(maxObjects * sizeof(BoundingBox), MEMTYPE_ANY);
	world->maxObjects = maxObjects;

	world->cameras = (Camera**)AllocMem(maxCameras * sizeof(Camera*), MEMTYPE_ANY);
	world->maxCameras = maxCameras;

	world->lights = (Light**)AllocMem(maxLights * sizeof(Light*), MEMTYPE_ANY);
	world->maxLights = maxLights;

	resetWorld(world, true, true);

	return world;
}

static void moveWorldObject(int srcIndex, int dstIndex, World *world)
{
	world->objects[dstIndex] = world->objects[srcIndex];
	world->objectInfo[dstIndex].priority = world->objectInfo[srcIndex].priority;
}

static void slotObjectIndexBasedOnPriority(Object3D *object, int priority, World *world)
{
	int i,j;

	for (i=0; i<world->nextObject; ++i) {
		if (world->objectInfo[i].priority > priority) break;
	}
	// i = the slot

	// move stuff
	for (j=world->nextObject; j>i; --j) {
		moveWorldObject(j-1, j, world);
	}
	world->nextObject++;

	//slot it
	world->objects[i] = object;
	world->objectInfo[i].priority = priority;
}

void addObjectToWorld(Object3D *object, int priority, World *world)
{
	const int objectIndex = world->nextObject;
	if (objectIndex < world->maxObjects) {
		slotObjectIndexBasedOnPriority(object, priority, world);
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

int findCurrentPrioritySize(World *world, int currentIndex)
{
	int i;
	const int currentPriority = world->objectInfo[currentIndex].priority;

	for (i=currentIndex+1; i<world->nextObject; ++i) {
		if (currentPriority != world->objectInfo[i].priority) break;
	}
	return i - currentIndex;
}

static void updateObjectWorldBoundingBox(int objectIndex, World *world, Camera *camera)
{
	static vec3f16 centerFromCam;

	Object3D *obj = world->objects[objectIndex];
	BoundingBox *srcBbox = &obj->bbox;
	BoundingBox *dstBbox = &world->objectBbox[objectIndex];

	copyVector3D(&srcBbox->halfSize, &dstBbox->halfSize);

	centerFromCam[0] = obj->pos.x + srcBbox->center.x - camera->pos.x;
	centerFromCam[1] = obj->pos.y + srcBbox->center.y - camera->pos.y;
	centerFromCam[2] = obj->pos.z + srcBbox->center.z - camera->pos.z;

	MulVec3Mat33_F16(centerFromCam, centerFromCam, camera->inverseRotMat);

	dstBbox->center.x = centerFromCam[0];
	dstBbox->center.y = centerFromCam[1];
	dstBbox->center.z = centerFromCam[2];
}

static void sortObjectByBoundingBoxZ(int objectIndex, BoundingBox *bbox)
{
	int n;
	const int z = bbox[objectIndex].center.z;

	// find insertion point
	int i = sortedObjectsNum;
	while(i > 0 && z < bbox[sortedObjectIndex[i-1]].center.z){--i;};

	// move stuff up to make space
	for (n=sortedObjectsNum; n>i; --n) {
		sortedObjectIndex[n] = sortedObjectIndex[n-1];
	}
	++sortedObjectsNum;

	// slot in index
	sortedObjectIndex[i] = objectIndex;
}

static void sortAndRenderObjects(int objectIndex, int num, World *world, Camera *camera)
{
	int i;
	const int nextIndex = objectIndex + num;

	sortedObjectsNum = 0;
	for (i=objectIndex; i<nextIndex; ++i) {
		updateObjectWorldBoundingBox(i, world, camera);
		sortObjectByBoundingBoxZ(i, world->objectBbox);
	}

	while(--sortedObjectsNum >= 0) {
		renderObject3D(world->objects[sortedObjectIndex[sortedObjectsNum]], camera, NULL, 0);
	};
}

void renderWorld(World *world)
{
	int currentIndex = 0;
	Camera *camera = world->cameras[world->activeCamera];

	createRotationMatrixValues(-camera->rot.x, -camera->rot.y, -camera->rot.z, (int*)camera->inverseRotMat);

	do {
		const int currentSize = findCurrentPrioritySize(world, currentIndex);
		sortAndRenderObjects(currentIndex, currentSize, world, camera);
		currentIndex += currentSize;
	} while(currentIndex < world->nextObject);
}
