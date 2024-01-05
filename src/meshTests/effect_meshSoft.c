#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_soft.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"

#include "sprite_engine.h"


static int rotX=10, rotY=20, rotZ=50;
//static int rotX=0, rotY=0, rotZ=0;
static int zoom=512;

static const int rotVel = 2;
static const int zoomVel = 2;

static Camera *camera;

static Object3D *softObj[4];

static int renderSoftMethodIndex = RENDER_SOFT_METHOD_GOURAUD;

static bool autoRot = false;

static int selectedSoftObj = 0;

 
static Object3D *initMeshObject(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex)
{
	Object3D *meshObj;

	Mesh *softMesh = initGenMesh(meshgenId, params, optionsFlags, tex);
	meshObj = initObject3D(softMesh);
	setObject3Dmesh(meshObj, softMesh);

	return meshObj;
}

static MeshgenParams initMeshObjectParams(int meshgenId)
{
	MeshgenParams params;

	switch(meshgenId) {
		case MESH_CUBE:
		{
			params = makeDefaultMeshgenParams(96);
		}
		break;

		case MESH_SQUARE_COLUMNOID:
		{
			int i;
			const int numPoints = 8;
			const int size = 64;
			Point2Darray *ptArray = initPoint2Darray(numPoints);

			for (i=0; i<numPoints; ++i) {
				const int y = (size/4) * (numPoints/2 - i);
				const int r = ((SinF16((i*20) << 16) * (size / 2)) >> 16) + size / 2;
				addPoint2D(ptArray, r,y);
			}
			params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

			//destroyPoint2Darray(ptArray); //why it crashes now?
		}
		break;
	}

	return params;
}


static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		rotX += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		rotX -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		rotY += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		rotY -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		rotZ += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		rotZ -= rotVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		autoRot = !autoRot;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_SELECT)) {
		selectedSoftObj = (selectedSoftObj+1) & 3;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		if (++renderSoftMethodIndex == RENDER_SOFT_METHOD_LAST) renderSoftMethodIndex = 0;

		setRenderSoftMethod(renderSoftMethodIndex);
	}
}


void effectMeshSoftInit()
{
	const int lightingOptions = MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP;
	MeshgenParams paramsCube = initMeshObjectParams(MESH_CUBE);
	MeshgenParams paramsColumnoid = initMeshObjectParams(MESH_SQUARE_COLUMNOID);

	const int texWidth = 64;
	const int texHeight = 64;

	Texture *cloudTex8 = initGenTexture(texWidth, texHeight, 8, NULL, 1, TEXGEN_CLOUDS, NULL);
	Texture *cloudTex16 = initGenTexture(texWidth, texHeight, 16, NULL, 1, TEXGEN_CLOUDS, NULL);

	softObj[0] = initMeshObject(MESH_CUBE, paramsCube, MESH_OPTION_RENDER_SOFT8 | lightingOptions, cloudTex8);
	softObj[1] = initMeshObject(MESH_SQUARE_COLUMNOID, paramsColumnoid, MESH_OPTION_RENDER_SOFT8 | lightingOptions, cloudTex8);
	softObj[2] = initMeshObject(MESH_CUBE, paramsCube, MESH_OPTION_RENDER_SOFT16 | lightingOptions, cloudTex16);
	softObj[3] = initMeshObject(MESH_SQUARE_COLUMNOID, paramsColumnoid, MESH_OPTION_RENDER_SOFT16 | lightingOptions, cloudTex16);

	setRenderSoftMethod(renderSoftMethodIndex);

	camera = createCamera();
}

static void renderSoftObj(int posX, int posZ, int t)
{
	setObject3Dpos(softObj[selectedSoftObj], -posX, 0, zoom + -posZ);
	if (autoRot) {
		setObject3Drot(softObj[selectedSoftObj], t<<1, t>>1, t);
	} else {
		setObject3Drot(softObj[selectedSoftObj], rotX, rotY, rotZ);
	}
	renderObject3D(softObj[selectedSoftObj], camera, NULL, 0);
}

void effectMeshSoftRun()
{
	const int t = getTicks() >> 5;

	inputScript();

	if ((softObj[selectedSoftObj]->mesh->renderType & MESH_OPTION_RENDER_SOFT8) && renderSoftMethodIndex==RENDER_SOFT_METHOD_WIREFRAME) {
		renderSoftMethodIndex = RENDER_SOFT_METHOD_GOURAUD;
		setRenderSoftMethod(renderSoftMethodIndex);
	}

	renderSoftObj(0, 256, t);
	
	//printf("test %d", t);

	displayDebugNums(false);
}
