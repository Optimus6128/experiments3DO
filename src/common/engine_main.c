#include "core.h"

#include "tools.h"
#include "cel_helpers.h"

#include "engine_view.h"
#include "engine_main.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "system_graphics.h"


#define Z_ORDER_SHIFT 4
#define Z_ORDER_SIZE (NUM_REC_Z >> Z_ORDER_SHIFT)

typedef struct zOrderListBucket
{
	CCB *first;
	CCB *last;
}zOrderListBucket;

static zOrderListBucket zOrderList[Z_ORDER_SIZE];
static int zIndexMin, zIndexMax;


static Vertex screenVertices[MAX_VERTEX_ELEMENTS_NUM];
static ScreenElement screenElements[MAX_VERTEX_ELEMENTS_NUM];
static Vector3D rotatedNormals[MAX_VERTEX_ELEMENTS_NUM];

static Light *globalLight = NULL;
static Vector3D rotatedGlobalLightVec;

static int screenOffsetX = 0;
static int screenOffsetY = 0;
static int screenWidth = SCREEN_WIDTH;
static int screenHeight = SCREEN_HEIGHT;

static bool polygonOrderTestCPU = true;

static void(*mapcelFunc)(CCB*, Point*, uint8);

CCB *startPolyCel;
CCB *endPolyCel;


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

static void useMapCelFunctionFast(bool enable)
{
	if (enable) {
		mapcelFunc = fasterMapCel;
	} else {
		mapcelFunc = slowMapCel;
	}
}

void useCPUtestPolygonOrder(bool enable)
{
	polygonOrderTestCPU = enable;
}

static void prepareTransformedMeshCELs(Mesh *mesh)
{
	int i;
	CCB *currentAddedCel = NULL;

	int *index = mesh->index;
	CCB *cel = mesh->cel;
	Vector3D *normal = mesh->polyNormal;
	const bool doPolyClipTests = !(mesh->renderType & MESH_OPTION_NO_POLYCLIP);
	const bool doPolySort = !(mesh->renderType & MESH_OPTION_NO_POLYSORT);

	if (doPolySort) {
		zIndexMin = Z_ORDER_SIZE-1;
		zIndexMax = 0;
	}

	startPolyCel = NULL;
	for (i=0; i<mesh->polysNum; ++i) {
		const int polyNumPoints = mesh->poly[i].numPoints;

		ScreenElement *sc1 = &screenElements[*index];
		ScreenElement *sc2 = &screenElements[*(index+1)];
		ScreenElement *sc3 = &screenElements[*(index+2)];
		ScreenElement *sc4;
		if (polyNumPoints == 3) {
			sc4 = sc3;
		} else {
			sc4 = &screenElements[*(index+3)];
		}

		if (!(doPolyClipTests && sc1->outside && sc2->outside && sc3->outside && sc4->outside) && 
			!(polygonOrderTestCPU && (sc1->x - sc2->x) * (sc3->y - sc2->y) - (sc3->x - sc2->x) * (sc1->y - sc2->y) <= 0)) {

				Point qpt[4];

				if (doPolySort) {	// prepare buckets and link CELs inside buckets
					const int avgZ = (sc1->z + sc2->z + sc3->z + sc4->z) >> (2 + Z_ORDER_SHIFT);
					zOrderListBucket *zBucket = &zOrderList[avgZ];

					if (zBucket->first==NULL) {
						if (avgZ < zIndexMin) zIndexMin = avgZ;
						if (avgZ > zIndexMax) zIndexMax = avgZ;
						zBucket->first = zBucket->last = cel;
					} else {
						cel->ccb_NextPtr = zBucket->first;
						zBucket->first = cel;
					}
				} else {
					if (!startPolyCel) {
						startPolyCel = currentAddedCel = cel;
					} else {
						currentAddedCel->ccb_NextPtr = cel;
						currentAddedCel = cel;
					}
				}

				if (mesh->renderType & MESH_OPTION_ENABLE_LIGHTING) {
					int shade = -(getVector3Ddot(normal, &rotatedGlobalLightVec) >> NORMAL_SHIFT);
					CLAMP(shade,(1<<(NORMAL_SHIFT-4)),((1<<NORMAL_SHIFT)-1))
					cel->ccb_PIXC = shadeTable[shade >> (NORMAL_SHIFT-SHADE_TABLE_SHR)];
				}

				qpt[0].pt_X = sc1->x; qpt[0].pt_Y = sc1->y;
				qpt[1].pt_X = sc2->x; qpt[1].pt_Y = sc2->y;
				qpt[2].pt_X = sc3->x; qpt[2].pt_Y = sc3->y;
				qpt[3].pt_X = sc4->x; qpt[3].pt_Y = sc4->y;
				mapcelFunc(cel, qpt, mesh->poly[i].texShifts);
		}
		index += polyNumPoints;
		++cel;
		++normal;
	}

	if (doPolySort) {	// Iterate through zBucket min/max to link the last CELs to next first CELs in the populated buckets
		zOrderListBucket *zBucket = &zOrderList[zIndexMax];
		CCB *prevLastCel = zBucket->last;

		int count = zIndexMax - zIndexMin;
		if (count < 0) return;

		startPolyCel = zBucket->first;
		zBucket->first = NULL;

		while(count-- > 0) {
			--zBucket;
			if (zBucket->first) {
				prevLastCel->ccb_NextPtr = zBucket->first;
				prevLastCel = zBucket->last;
				zBucket->first = NULL;
			}
		};
		endPolyCel = zBucket->last;
		endPolyCel->ccb_Flags |= CCB_LAST;
	} else if (currentAddedCel) {
		currentAddedCel->ccb_Flags |= CCB_LAST;
		endPolyCel = currentAddedCel;
	}
}

static void calculateVertexLighting(Mesh *mesh)
{
	int i;
	const int verticesNum = mesh->verticesNum;
	Vector3D *normal = mesh->vertexNormal;

	for (i=0; i<verticesNum; ++i) {
		const int light = -(getVector3Ddot(normal, &rotatedGlobalLightVec) >> NORMAL_SHIFT);
		int c = light >> (NORMAL_SHIFT-COLOR_GRADIENTS_SHR);
		CLAMP(c,1,COLOR_GRADIENTS_SIZE-2)
		screenElements[i].c = c;
		++normal;
	}
}

static void calculateVertexEnvmapTC(Mesh *mesh)
{
	int i;
	const int verticesNum = mesh->verticesNum;
	Texture *tex = &mesh->tex[0];

	const int texWidthHalf = tex->width >> 1;
	const int texHeightHalf = tex->height >> 1;
	const int wShiftHalf = NORMAL_SHIFT - tex->wShift + 1;
	const int hShiftHalf = NORMAL_SHIFT - tex->hShift + 1;

	for (i=0; i<verticesNum; ++i) {
		int normZ = rotatedNormals[i].z;
		if (normZ != 0) {
			int normX = (rotatedNormals[i].x>>wShiftHalf) + texWidthHalf;
			int normY = (rotatedNormals[i].y>>hShiftHalf) + texHeightHalf;

			screenElements[i].u = normX;
			screenElements[i].v = normY;
		}
	}
}

static void transposeMat3(mat33f16 mat)
{
	int temp;
	int *matVal = (int*)mat;

	temp = matVal[1]; matVal[1] = matVal[3]; matVal[3] = temp;
	temp = matVal[2]; matVal[2] = matVal[6]; matVal[6] = temp;
	temp = matVal[5]; matVal[5] = matVal[7]; matVal[7] = temp;
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

static void translateAndProjectVertices(Object3D *obj, Camera *cam)
{
	static vec3f16 posFromCam;

	int i;

	const int lvNum = obj->mesh->verticesNum;

	const int screenWidthHalf = screenWidth >> 1;
	const int screenHeightHalf = screenHeight >> 1;
	const int offsetX = screenOffsetX + screenWidthHalf;
	const int offsetY = screenOffsetY + screenHeightHalf;
	const bool doPolyClipTests = !(obj->mesh->renderType & MESH_OPTION_NO_POLYCLIP);

	posFromCam[0] = obj->pos.x - cam->pos.x;
	posFromCam[1] = obj->pos.y - cam->pos.y;
	posFromCam[2] = obj->pos.z - cam->pos.z;

	MulVec3Mat33_F16(posFromCam, posFromCam, cam->inverseRotMat);

	for (i=0; i<lvNum; i++) {
		int edgeX, edgeY;
		int vx = screenVertices[i].x + posFromCam[0];
		int vy = screenVertices[i].y + posFromCam[1];
		int vz = screenVertices[i].z + posFromCam[2];
		CLAMP(vz, cam->near, cam->far)

		if (doPolyClipTests) {
			edgeX = (screenWidthHalf * vz) >> PROJ_SHR;
			edgeY = (screenHeightHalf * vz) >> PROJ_SHR;
			screenElements[i].outside = (vx < -edgeX || vx > edgeX || vy < -edgeY || vy > edgeY);
		}

		screenElements[i].x = offsetX + (((vx << PROJ_SHR) * recZ[vz]) >> REC_FPSHR);
		screenElements[i].y = offsetY - (((vy << PROJ_SHR) * recZ[vz]) >> REC_FPSHR);
		screenElements[i].z = vz;
	}
}

static void transformMesh(Object3D *obj, Camera *cam)
{
	static mat33f16 rotMat;
	static mat33f16 rotViewMat;

	Mesh *mesh = obj->mesh;

	createRotationMatrixValues(obj->rot.x, obj->rot.y, obj->rot.z, (int*)rotMat);

	MulMat33Mat33_F16(rotViewMat, rotMat, cam->inverseRotMat);


	// Rotate Mesh Vertices
	MulManyVec3Mat33_F16((vec3f16*)screenVertices, (vec3f16*)mesh->vertex, rotViewMat, mesh->verticesNum);

	translateAndProjectVertices(obj, cam);

	if (mesh->renderType & MESH_OPTION_RENDER_SOFT) {
		if (mesh->renderType & MESH_OPTION_ENABLE_ENVMAP) {
			MulManyVec3Mat33_F16((vec3f16*)rotatedNormals, (vec3f16*)mesh->vertexNormal, rotViewMat, mesh->verticesNum);
		}
	}
	if (mesh->renderType & MESH_OPTION_ENABLE_LIGHTING) {
		transposeMat3(rotMat);
		normalizeVector3D(&globalLight->dir);
		MulManyVec3Mat33_F16((vec3f16*)&rotatedGlobalLightVec, (vec3f16*)&globalLight->dir, rotMat, 1);
	}
}

static void renderTransformedMesh(Mesh *mesh)
{
	useMapCelFunctionFast(mesh->renderType & MESH_OPTION_FAST_MAPCEL);
	useCPUtestPolygonOrder(mesh->renderType & MESH_OPTION_CPU_POLYTEST);

	prepareTransformedMeshCELs(mesh);

	if (startPolyCel) {
		drawCels(startPolyCel);
		endPolyCel->ccb_Flags &= ~CCB_LAST;
	}
}

// Multiple lights not implemented yet, just add stub arguments
void renderObject3D(Object3D *obj, Camera *cam, Light **lights, int lightsNum)
{
	Mesh *mesh = obj->mesh;

	transformMesh(obj, cam);

	if (mesh->renderType & MESH_OPTION_RENDER_SOFT) {
		if (mesh->renderType & MESH_OPTION_ENABLE_LIGHTING) {
			calculateVertexLighting(mesh);
		}
		if (mesh->renderType & MESH_OPTION_ENABLE_ENVMAP) {
			calculateVertexEnvmapTC(mesh);
		}
		renderTransformedMeshSoft(mesh, screenElements);
	} else {
		renderTransformedMesh(mesh);
	}
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

static void calculateBoundingBox(Object3D* obj)
{
	int i;
	Vector3D min, max;
	Mesh *mesh = obj->mesh;
	BoundingBox *bbox = &obj->bbox;

	for (i=0; i<mesh->verticesNum; ++i) {
		Vertex *v = &mesh->vertex[i];
		if (i==0) {
			min.x = max.x = v->x;
			min.y = max.y = v->y;
			min.z = max.y = v->z;
		} else {
			if (v->x < min.x) min.x = v->x;
			if (v->y < min.y) min.y = v->y;
			if (v->z < min.z) min.z = v->z;
			if (v->x > max.x) max.x = v->x;
			if (v->y > max.y) max.y = v->y;
			if (v->z > max.z) max.z = v->z;
		}
	}

	addVector3D(&bbox->center, &min, &max);
	divScalarVector3D(&bbox->center, 2);

	subVector3D(&bbox->halfSize, &max, &min);
	divScalarVector3D(&bbox->halfSize, 2);

	// Keep to see what breaks the compiler in the future
	//bbox->diagonalLength = getVector3Dlength(&bbox->halfSize);
}

Object3D* initObject3D(Mesh *ms)
{
	Object3D *obj = (Object3D*)AllocMem(sizeof(Object3D), MEMTYPE_ANY);

	obj->mesh = ms;
	
	obj->pos.x = obj->pos.y = obj->pos.z = 0;
	obj->rot.x = obj->rot.y = obj->rot.z = 0;
	
	calculateBoundingBox(obj);

	// removed it from calculateBoundingBox at the end as the compiler produces wrong code? (app freezes for the same thing as here)
	obj->bbox.diagonalLength = getVector3Dlength(&obj->bbox.halfSize);

	return obj;
}

void setObject3Dpos(Object3D *obj, int px, int py, int pz)
{
	obj->pos.x = px;
	obj->pos.y = py;
	obj->pos.z = pz;
}

void setObject3Drot(Object3D *obj, int rx, int ry, int rz)
{
	obj->rot.x = rx;
	obj->rot.y = ry;
	obj->rot.z = rz;
}

void setObject3Dmesh(Object3D *obj, Mesh *ms)
{
	obj->mesh = ms;
}


void setLightPos(Light *light, int px, int py, int pz)
{
	light->pos.x = px;
	light->pos.y = py;
	light->pos.z = pz;
}

void setLightDir(Light *light, int vx, int vy, int vz)
{
	light->dir.x = vx;
	light->dir.y = vy;
	light->dir.z = vz;
}

Light *createLight(bool isDirectional)
{
	Light *light = (Light*)AllocMem(sizeof(Light), MEMTYPE_ANY);

	setLightPos(light, 0,1024,0);
	setLightDir(light, 0,-(1 << NORMAL_SHIFT), 0);

	return light;
}

void setGlobalLight(Light *light)
{
	globalLight = light;
}


void initEngine(bool usesSoftEngine)
{
	initEngineLUTs();

	useCPUtestPolygonOrder(false);
	useMapCelFunctionFast(true);

	globalLight = createLight(true);
	setLightDir(globalLight, -1,-1,1);

	memset(zOrderList, 0, sizeof(zOrderListBucket) * Z_ORDER_SIZE);

	if (usesSoftEngine) initEngineSoft();
}
