#include "engine_view.h"

#include "input.h"
#include "engine_main.h"
#include "mathutil.h"


void setCameraPos(Camera *cam, int px, int py, int pz)
{
	setVector3D(&cam->pos, px, py, pz);
}

void setCameraRot(Camera *cam, int rx, int ry, int rz)
{
	setVector3D(&cam->rot, rx, ry, rz);
}

void updateCameraMatrix(Camera *cam)
{
	createRotationMatrixValues(-cam->rot.x, -cam->rot.y, -cam->rot.z, (int*)cam->inverseRotMat);
}

Camera *createCamera()
{
	Camera *cam = (Camera*)AllocMem(sizeof(Camera), MEMTYPE_ANY);

	setCameraPos(cam, 0,0,0);
	setCameraRot(cam, 0,0,0);

	cam->near = 16;
	cam->far = NUM_REC_Z-1;

	updateCameraMatrix(cam);

	return cam;
}

static void updateCameraInViewer(Viewer *viewer)
{
	const int headPosY = viewer->bbox.center.y - viewer->bbox.halfSize.y + viewer->viewHeight;
	setCameraPos(viewer->camera, viewer->pos.x>>FP_VPOS, (viewer->pos.y>>FP_VPOS) + headPosY, viewer->pos.z>>FP_VPOS);
	setCameraRot(viewer->camera, viewer->rot.x>>FP_VROT, viewer->rot.y>>FP_VROT, viewer->rot.z>>FP_VROT);
}

void setViewerPos(Viewer *viewer, int px, int py, int pz)
{
	setVector3D(&viewer->pos, px<<FP_VPOS, py<<FP_VPOS, pz<<FP_VPOS);
	updateCameraInViewer(viewer);
}

void setViewerRot(Viewer *viewer, int rx, int ry, int rz)
{
	setVector3D(&viewer->rot, rx<<FP_VROT, ry<<FP_VROT, rz<<FP_VROT);
	updateCameraInViewer(viewer);
}

static void rotateViewer(int rx, int ry, int rz, int dt, Viewer *viewer)
{
	const int camRotLimX = 60<<FP_VROT;

	switch(rx){
		case -1:
			if (viewer->rot.x > -camRotLimX) {
				viewer->rot.x -= (viewer->rotSpeed * dt);
			}
		break;
		
		case 1:
			if (viewer->rot.x < camRotLimX) {
				viewer->rot.x += (viewer->rotSpeed * dt);
			}
		break;
	}

	switch(ry){
		case -1:
			viewer->rot.y -= (viewer->rotSpeed * dt);
		break;
		
		case 1:
			viewer->rot.y += (viewer->rotSpeed * dt);
		break;
	}
}

static bool tryCollideMoveStep(bool x, bool y, vec3f16 move, Viewer *viewer, int dt)
{
	// test collide
	const int off = 128 << FP_VPOS;
	Vector3D *viewerPos = &viewer->pos;

	int prevCamPosX = viewerPos->x;
	int prevCamPosZ = viewerPos->z;

	if (x) viewerPos->x += move[0] * viewer->moveSpeed * dt;
	if (y) viewerPos->z += move[2] * viewer->moveSpeed * dt;

	if (viewerPos->x>-off && viewerPos->x<off && viewerPos->z>-off && viewerPos->z<off) {
		viewerPos->x = prevCamPosX;
		viewerPos->z = prevCamPosZ;
		return true;
	}
	return false;
}

static void moveStep(bool x, bool y, vec3f16 move, Viewer *viewer, int dt)
{
	Vector3D *viewerPos = &viewer->pos;

	if (x) viewerPos->x += move[0] * viewer->moveSpeed * dt;
	if (y) viewerPos->z += move[2] * viewer->moveSpeed * dt;
}

static void moveViewer(int forward, int right, int up, int dt, Viewer *viewer)
{
	static bool collideTest = false;
	static mat33f16 rotMat;
	static vec3f16 move;
	int prevPosY, feetPosY;

	move[0] = right << (FP_VPOS - 1);
	move[1] = 0;
	move[2] = forward << FP_VPOS;

	createRotationMatrixValues(0, viewer->rot.y >> FP_VROT, 0, (int*)rotMat);	// not correct when looking up/down yet
	MulVec3Mat33_F16(move, move, rotMat);

	if (collideTest) {
		if (tryCollideMoveStep(true, true, move, viewer, dt)) {
			if(tryCollideMoveStep(true, false, move, viewer, dt)) {
				tryCollideMoveStep(false, true, move, viewer, dt);
			}
		}
	} else {
		moveStep(true, true, move, viewer, dt);
	}

	prevPosY = viewer->pos.y;
	viewer->pos.y += (up << (FP_VPOS - 2)) * viewer->flySpeed * dt;
	feetPosY = (viewer->pos.y >> FP_VPOS) + viewer->bbox.center.y - viewer->bbox.halfSize.y;
	//if (feetPosY <0) viewer->pos.y = prevPosY;
}

void viewerInputFPS(Viewer *viewer, int dt)
{
	bool cameraLook = isJoyButtonPressed(JOY_BUTTON_C);

	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		rotateViewer(0,1,0, dt,viewer);
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		rotateViewer(0,-1,0, dt,viewer);
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		if (cameraLook) {
			rotateViewer(-1,0,0, dt,viewer);
		} else {
			moveViewer(1,0,0, dt,viewer);
		}
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		if (cameraLook) {
			rotateViewer(1,0,0, dt,viewer);
		} else {
			moveViewer(-1,0,0, dt,viewer);
		}
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		moveViewer(0,0,1, dt,viewer);
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		moveViewer(0,0,-1, dt,viewer);
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		moveViewer(0,-1,0, dt,viewer);
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		moveViewer(0,1,0, dt,viewer);
	}

	updateCameraInViewer(viewer);
}


Viewer *createViewer(int width, int height, int depth, int viewHeight)
{
	Viewer *viewer = AllocMem(sizeof(Viewer), MEMTYPE_ANY);

	viewer->camera = createCamera();
	viewer->viewHeight = viewHeight;

	viewer->rotSpeed = 1 << (FP_VROT - 4);
	viewer->moveSpeed = 1;
	viewer->flySpeed = 2;

	setVector3D(&viewer->pos, 0,0,0);
	setVector3D(&viewer->rot, 0,0,0);

	setVector3D(&viewer->bbox.center, 0,0,0);
	setVector3D(&viewer->bbox.halfSize, width/2, height/2, depth/2);

	updateCameraInViewer(viewer);

	return viewer;
}
