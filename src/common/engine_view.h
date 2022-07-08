#ifndef ENGINE_VIEW_H
#define ENGINE_VIEW_H

#include "core.h"

#include "types.h"
#include "mathutil.h"

#define FP_VPOS 16
#define FP_VROT 16

typedef struct Camera
{
	Vector3D pos, rot;
	int near, far;
	mat33f16 inverseRotMat;
}Camera;

typedef struct Viewer
{
	Vector3D pos, rot;
	BoundingBox bbox;
	int viewHeight;

	Camera *camera;

	int rotSpeed;
	int moveSpeed;
	int flySpeed;
}Viewer;

Camera *createCamera(void);
void updateCameraMatrix(Camera *cam);
void setCameraPos(Camera *cam, int px, int py, int pz);
void setCameraRot(Camera *cam, int rx, int ry, int rz);

Viewer *createViewer(int width, int height, int depth, int viewHeight);
void setViewerPos(Viewer *viewer, int px, int py, int pz);
void setViewerRot(Viewer *viewer, int rx, int ry, int rz);

void viewerInputFPS(Viewer *viewer, int dt);

#endif
