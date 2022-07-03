#include "core.h"

#include "tools.h"
#include "cel_helpers.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_main.h"
#include "engine_soft.h"

#include "system_graphics.h"

#include "mathutil.h"

static Vertex screenVertices[MAX_VERTEX_ELEMENTS_NUM];
static ScreenElement screenElements[MAX_VERTEX_ELEMENTS_NUM];
static Vector3D rotatedNormals[MAX_VERTEX_ELEMENTS_NUM];

static int screenOffsetX = 0;
static int screenOffsetY = 0;
static int screenWidth = SCREEN_WIDTH;
static int screenHeight = SCREEN_HEIGHT;

static bool polygonOrderTestCPU = true;

static void(*mapcelFunc)(CCB*, Point*, uint8);


int shadeTable[SHADE_TABLE_SIZE] = {
 0x03010301,0x07010701,0x0B010B01,0x0F010F01,0x13011301,0x17011701,0x1B011B01,0x1F011F01,
 0x03C103C1,0x07C107C1,0x0BC10BC1,0x0FC10FC1,0x13C113C1,0x17C117C1,0x1BC11B01,0x1FC11FC1
};

static void slowMapCel(CCB *c, Point *q, uint8 texShifts)
{
	MapCel(c, q);
}

static void fasterMapCel(CCB *c, Point *q, uint8 texShifts)
{
	const int shrWidth = (int)(texShifts >> 4);
	const int shrHeight = (int)(texShifts & 15);

	const int q0x = q[0].pt_X;
	const int q0y = q[0].pt_Y;
	const int q1x = q[1].pt_X;
	const int q1y = q[1].pt_Y;
	const int q2x = q[2].pt_X;
	const int q2y = q[2].pt_Y;
	const int q3x = q[3].pt_X;
	const int q3y = q[3].pt_Y;

	const int ptX0 = q1x - q0x;
	const int ptY0 = q1y - q0y;
	const int ptX1 = q2x - q3x;
	const int ptY1 = q2y - q3y;
	const int ptX2 = q3x - q0x;
	const int ptY2 = q3y - q0y;

	const int hdx0 = (ptX0 << 20) >> shrWidth;
	const int hdy0 = (ptY0 << 20) >> shrWidth;
	const int hdx1 = (ptX1 << 20) >> shrWidth;
	const int hdy1 = (ptY1 << 20) >> shrWidth;

	c->ccb_XPos = q0x << 16;
	c->ccb_YPos = q0y << 16;

	c->ccb_HDX = hdx0;
	c->ccb_HDY = hdy0;
	c->ccb_VDX = (ptX2 << 16) >> shrHeight;
	c->ccb_VDY = (ptY2 << 16) >> shrHeight;

	c->ccb_HDDX = (hdx1 - hdx0) >> shrHeight;
	c->ccb_HDDY = (hdy1 - hdy0) >> shrHeight;
}

void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *rotVecs)
{
	const int cosxr = isin[(rotX + 64) & 255];
	const int cosyr = isin[(rotY + 64) & 255];
	const int coszr = isin[(rotZ + 64) & 255];
	const int sinxr = isin[rotX & 255];
	const int sinyr = isin[rotY & 255];
	const int sinzr = isin[rotZ & 255];

	*rotVecs++ = (FIXED_MUL(cosyr, coszr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(FIXED_MUL(sinxr, sinyr, FP_BASE), coszr, FP_BASE) - FIXED_MUL(cosxr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(FIXED_MUL(cosxr, sinyr, FP_BASE), coszr, FP_BASE) + FIXED_MUL(sinxr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(cosyr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(cosxr, coszr, FP_BASE) + FIXED_MUL(FIXED_MUL(sinxr, sinyr, FP_BASE), sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(-sinxr, coszr, FP_BASE) + FIXED_MUL(FIXED_MUL(cosxr, sinyr, FP_BASE), sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (-sinyr) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(sinxr, cosyr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs = (FIXED_MUL(cosxr, cosyr, FP_BASE)) << FP_BASE_TO_CORE;
}

static void translateAndProjectVertices(Object3D *obj)
{
	int i;

	const int posX = obj->posX;
	const int posY = obj->posY;
	const int posZ = obj->posZ;

	const int lvNum = obj->mesh->verticesNum;

	const int offsetX = screenOffsetX + (screenWidth >> 1);
	const int offsetY = screenOffsetY + (screenHeight >> 1);

	for (i=0; i<lvNum; i++)
	{
		const int vz = screenVertices[i].z + posZ;
		if (vz > 0) {
			const int recDivZ = recZ[vz];
			screenElements[i].x = offsetX + ((((screenVertices[i].x + posX) << PROJ_SHR) * recDivZ) >> REC_FPSHR);
			screenElements[i].y = offsetY - ((((screenVertices[i].y + posY) << PROJ_SHR) * recDivZ) >> REC_FPSHR);
		}
		screenElements[i].z = vz;
	}
}

static void rotateVerticesHw(Object3D *obj, bool rotatePolyNormals, bool rotateVertexNormals)
{
	mat33f16 rotMat;

	createRotationMatrixValues(obj->rotX, obj->rotY, obj->rotZ, (int*)rotMat);

	MulManyVec3Mat33_F16((vec3f16*)screenVertices, (vec3f16*)obj->mesh->vertex, rotMat, obj->mesh->verticesNum);

	if (obj->mesh->renderType & MESH_OPTION_ENABLE_LIGHTING) {
		// in the future, we might not need to rotate the normals but rather the light against the normals
		// so we won't need the normals container, as we will test light vector against original mesh normals
		if (rotatePolyNormals) {
			MulManyVec3Mat33_F16((vec3f16*)rotatedNormals, (vec3f16*)obj->mesh->polyNormal, rotMat, obj->mesh->polysNum); 
		}
		if (rotateVertexNormals) {
			MulManyVec3Mat33_F16((vec3f16*)rotatedNormals, (vec3f16*)obj->mesh->vertexNormal, rotMat, obj->mesh->verticesNum);
		}
	}
}

static void prepareTransformedMeshCELs(Mesh *ms)
{
	int i;
	int *index = ms->index;
	Point qpt[4];
	CCB *cel = ms->cel;

	for (i=0; i<ms->polysNum; ++i) {
		bool allOut;
		int n = 1;
		const int zt = 128;
		int z0, z1, z2, z3 = -1;
		qpt[0].pt_X = screenElements[*index].x; qpt[0].pt_Y = screenElements[*index].y; z0 = screenElements[*index].z; ++index;
		qpt[1].pt_X = screenElements[*index].x; qpt[1].pt_Y = screenElements[*index].y; z1 = screenElements[*index].z; ++index;
		qpt[2].pt_X = screenElements[*index].x; qpt[2].pt_Y = screenElements[*index].y; z2 = screenElements[*index].z; ++index;

		// Handling quads or triangles for now.
		if (ms->poly[i].numPoints == 4) {
			qpt[3].pt_X = screenElements[*index].x; qpt[3].pt_Y = screenElements[*index].y; z3 = screenElements[*index].z; ++index;
		} else {
			qpt[3].pt_X = qpt[2].pt_X; qpt[3].pt_Y = qpt[2].pt_Y;
		}
		
		allOut = (z0 < zt && z1 < zt && z2 < zt && z3 < zt);

		if (!allOut && polygonOrderTestCPU) {
			n = (qpt[0].pt_X - qpt[1].pt_X) * (qpt[2].pt_Y - qpt[1].pt_Y) - (qpt[2].pt_X - qpt[1].pt_X) * (qpt[0].pt_Y - qpt[1].pt_Y);
		}

		if (allOut || n <= 0) {
			cel->ccb_Flags |= CCB_SKIP;
		} else {
			cel->ccb_Flags &= ~CCB_SKIP;

			if (ms->renderType & MESH_OPTION_ENABLE_LIGHTING) {
				int normZ = -rotatedNormals[i].z;
				CLAMP(normZ,0,((1<<NORMAL_SHIFT)-1))
				cel->ccb_PIXC = shadeTable[normZ >> (NORMAL_SHIFT-SHADE_TABLE_SHR)];
			}

			mapcelFunc(cel, qpt, ms->poly[i].texShifts);
		}
		++cel;
	}
}

static void calculateVertexLighting(Object3D *obj)
{
	int i, c;
	const int verticesNum = obj->mesh->verticesNum;

	for (i=0; i<verticesNum; ++i) {
		int normZ = -rotatedNormals[i].z;
		CLAMP(normZ,0,((1<<NORMAL_SHIFT)-1))
		c = normZ >> (NORMAL_SHIFT-COLOR_GRADIENTS_SHR);
		CLAMP(c,1,COLOR_GRADIENTS_SIZE-2)
		screenElements[i].c = c;
	}
}

static void calculateVertexEnvmapTC(Object3D *obj)
{
	int i;
	const int verticesNum = obj->mesh->verticesNum;
	Texture *tex = &obj->mesh->tex[0];

	const int texWidthHalf = tex->width >> 1;
	const int texHeightHalf = tex->height >> 1;
	const int wShiftHalf = NORMAL_SHIFT - tex->wShift + 1;
	const int hShiftHalf = NORMAL_SHIFT - tex->hShift + 1;

	for (i=0; i<verticesNum; ++i) {
		int normZ = rotatedNormals[i].z;
		if (normZ != 0) {
			int normX = (rotatedNormals[i].x>>wShiftHalf) + texWidthHalf;
			int normY = (rotatedNormals[i].y>>hShiftHalf) + texHeightHalf;

//			normX &= (texWidth - 1);
//			normY &= (texHeight - 1);

			screenElements[i].u = normX;
			screenElements[i].v = normY;
		}
	}
}

static void useMapCelFunctionFast(bool enable)
{
	if (enable) {
		mapcelFunc = fasterMapCel;
	} else {
		mapcelFunc = slowMapCel;
	}
}

static void transformMesh(Object3D *obj, bool soft)
{
	rotateVerticesHw(obj, !soft, soft);
	translateAndProjectVertices(obj);
}

static void renderTransformedMesh(Object3D *obj)
{
	//useMapCelFunctionFast(false);	// Should deduce it or maybe mark the polygons if textures are power of two. Using the slower option for now.
	useMapCelFunctionFast(true);	// Will now see what it breaks...
	useCPUtestPolygonOrder(false);	// False for now for the grid effect..
	//useCPUtestPolygonOrder(true);	// the CEL clockwise clipping sucks anyway. In the future we my add this option as a state or per object maybe..

	prepareTransformedMeshCELs(obj->mesh);
	drawCels(obj->mesh->cel);
}

void renderObject3D(Object3D *obj)
{
	transformMesh(obj, false);
	renderTransformedMesh(obj);
}

void renderObject3Dsoft(Object3D *obj)
{
	transformMesh(obj, true);

	if (obj->mesh->renderType & MESH_OPTION_ENABLE_LIGHTING) {
		calculateVertexLighting(obj);
	}
	if (obj->mesh->renderType & MESH_OPTION_ENABLE_ENVMAP) {
		calculateVertexEnvmapTC(obj);
	}
	renderTransformedMeshSoft(obj->mesh, screenElements);
}

void setScreenRegion(int posX, int posY, int width, int height)
{
	// In the future I can do some of it origin clipping on the hardware API
	// however the new width/height is still needed for the CPU to offset the 3d transformations to the center

	// Doom used SetClipWidth, SetClipHeight, SetClipOrigin but in other code like the STNICCC I couldn't make them work, maybe need some more init somewhere else

	screenOffsetX = posX;
	screenOffsetY = posY;
	screenWidth = width;
	screenHeight = height;
}

Object3D* initObject3D(Mesh *ms)
{
	Object3D *obj = (Object3D*)AllocMem(sizeof(Object3D), MEMTYPE_ANY);

	obj->mesh = ms;
	
	obj->posX = obj->posY = obj->posZ = 0;
	obj->rotX = obj->rotY = obj->rotZ = 0;

	return obj;
}

void setObject3Dpos(Object3D *obj, int px, int py, int pz)
{
	obj->posX = px;
	obj->posY = py;
	obj->posZ = pz;
}

void setObject3Drot(Object3D *obj, int rx, int ry, int rz)
{
	obj->rotX = rx;
	obj->rotY = ry;
	obj->rotZ = rz;
}

void setObject3Dmesh(Object3D *obj, Mesh *ms)
{
	obj->mesh = ms;
}

void useCPUtestPolygonOrder(bool enable)
{
	polygonOrderTestCPU = enable;
}

void initEngine(bool usesSoftEngine)
{
	initEngineLUTs();

	useCPUtestPolygonOrder(false);
	useMapCelFunctionFast(true);

	if (usesSoftEngine) initEngineSoft();
}
