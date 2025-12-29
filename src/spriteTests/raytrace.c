#include "core.h"

#include "math.h"
#include "string.h"
#include "stdio.h"

#include "system_graphics.h"
#include "tools.h"

#include "vectorFp.h"
#include "raytrace.h"
#include "mathutil.h"


#define FAR_FI (1 << 30)

#define RT_BUFF_WIDTH (RT_WIDTH + 1)
#define RT_BUFF_HEIGHT (RT_HEIGHT + 1)

#define HIT_BUFF_WIDTH ((RT_WIDTH / 2) + 1)
#define HIT_BUFF_HEIGHT ((RT_HEIGHT / 2) + 1)

typedef struct Object
{
	int type;
	Vec3fp pos, dir1, dir2;
	int radius;
	int invR;
	int a, b, c, d;
	int index;
} Object;

typedef struct Hit
{
	Object* obj;
	Vec3fp pos;
	Vec3fp normal;
} Hit;

typedef struct PerPixelVars
{
	int dirDot, invDirDot;
}PerPixelVars;

typedef struct PerPixelPrecs
{
	Vec3fp viewDir[RT_BUFF_WIDTH * RT_BUFF_HEIGHT];
	PerPixelVars vars[RT_BUFF_WIDTH * RT_BUFF_HEIGHT];
} PerPixelPrecs;

static Object nothing;
static Object plane;
static Object sphere[3];
static Vec3fp lightDir0;

static PerPixelPrecs perPixelPrecs;

static Hit hitBuffer[HIT_BUFF_WIDTH * HIT_BUFF_HEIGHT];

static int planeYt[RT_BUFF_HEIGHT];
static int planeTicks;

static uint16 fbVgaPal[256];


static void setupPlaneYprecs()
{
	int y;
	Object* obj = &plane;

	for (y = 0; y < RT_BUFF_HEIGHT; ++y) {
		Vec3fp* dir = &perPixelPrecs.viewDir[y * RT_BUFF_WIDTH];
		const int denom = FP_MUL_T(obj->a, dir->x) + FP_MUL_T(obj->b, dir->y) + FP_MUL_T(obj->c, dir->z);
		int t = -1;
		if (denom != 0) {
			t = -FP_DIV_T(obj->d, denom);
		}
		planeYt[y] = t;
	}
}

static void setupViewDirPerPixelLUTs()
{
	int x,y;

	const float aspectRatio = (float)(RT_HEIGHT) / (float)RT_WIDTH;

	const int Xinc = FLT_TO_FP(2.0f / (RT_WIDTH - 1));
	const int Yinc = FLT_TO_FP((2.0f / (RT_HEIGHT - 1)) * aspectRatio);

	Vec3fp* dir = perPixelPrecs.viewDir;
	PerPixelVars* perPixelVars = perPixelPrecs.vars;

	int Ydir = (RT_HEIGHT / 2) * Yinc;
	for (y = 0; y <= RT_HEIGHT; y++) {
		int Xdir = (-RT_WIDTH / 2) * Xinc;
		for (x = 0; x <= RT_WIDTH; x++) {
			int dd;
			dir->x = Xdir;
			dir->y = Ydir;
			dir->z = INT_TO_FP(1);

			//Don't normalize yet, if we can get the vis without it..
			//Vnormalize(dir);

			dd = 2 * (FP_MUL_T(dir->x, dir->x) + FP_MUL_T(dir->y, dir->y) + FP_MUL_T(dir->z, dir->z));

			perPixelVars->dirDot = dd;
			if (dd == 0) dd = 1;	// move a bit just in case to avoid division by zero (didn't happen though)
			perPixelVars->invDirDot = FP_DIV_T(FLT_TO_FP(1.0f), dd);

			perPixelVars++;
			dir++;
			Xdir += Xinc;
		}
		Ydir -= Yinc;
	}
}

static int Vdot(Vec3fp* v1, Vec3fp* v2)
{
    return FP_MUL_T(v1->x, v2->x) + FP_MUL_T(v1->y, v2->y) + FP_MUL_T(v1->z, v2->z);
}

static int Vlength(Vec3fp *v)
{
	return FP_SQRT(Vdot(v, v));
}

static void Vnormalize (Vec3fp* v)
{
	int r = Vlength(v);
	if (r!=0) {
		v->x = FP_DIV_T(v->x, r);
		v->y = FP_DIV_T(v->y, r);
		v->z = FP_DIV_T(v->z, r);
	}
}

static void Vcross(Vec3fp* v1, Vec3fp* v2, Vec3fp* v)
{
    v->x = FP_MUL_T(v1->y, v2->z) - FP_MUL_T(v2->y, v1->z);
    v->y = FP_MUL_T(v1->z, v2->x) - FP_MUL_T(v2->z, v1->x);
    v->z = FP_MUL_T(v1->x, v2->y) - FP_MUL_T(v2->x, v1->y);
}

static void calcABCD(Object* obj)
{
	Vec3fp v;
	switch (obj->type) {

		case OBJ_PLANE:
			Vcross(&obj->dir1, &obj->dir2, &v);
			Vnormalize(&v);
			obj->a = v.x;
			obj->b = v.y;
			obj->c = v.z;
			obj->d = -Vlength(&obj->pos);
			break;

		case OBJ_SPHERE:
			obj->a = obj->pos.x;
			obj->b = obj->pos.y;
			obj->c = obj->pos.z;
			obj->d = 2 * (FP_MUL_T(obj->pos.x, obj->pos.x) + FP_MUL_T(obj->pos.y, obj->pos.y) + FP_MUL_T(obj->pos.z, obj->pos.z) - FP_MUL_T(obj->radius, obj->radius));
			break;

		default:
			break;
	}
}

/*static int intersectPlane(Object* obj, Vec3fp* dir)
{
	const int denom = FP_MUL_T(obj->a, dir->x) + FP_MUL_T(obj->b, dir->y) + FP_MUL_T(obj->c, dir->z);
	if (denom!=0) {
		return -FP_DIV_T(obj->d, denom);
	}
	return -1;
}*/

static int intersectSphere(Object* obj, Vec3fp* dir, PerPixelVars *perPixelVars)
{
	const int b = 2 * (FP_MUL_T(dir->x, -obj->pos.x) + FP_MUL_T(dir->y, -obj->pos.y) + FP_MUL_T(dir->z, -obj->pos.z));
	const int c = obj->d;

	int d = FP_MUL_T(b, b) - FP_MUL_T(perPixelVars->dirDot, c);
	if (d >= 0) {
		int t;
		int invDirDot = perPixelVars->invDirDot;
		d = FP_SQRT(d);

		t = FP_MUL_T(-b - d, invDirDot);
		if (t <= 0) {
			t = FP_MUL_T(-b + d, invDirDot);
		}
		return t;
	}
	return -1;
}

static void traceRay(Vec3fp *dir, PerPixelVars *perPixelVars, Hit *hit, int y)
{
	int t, tMin = FAR_FI;
	Object* obj = &nothing;

	t = intersectSphere(&sphere[0], dir, perPixelVars); if (t > 0 && t < tMin) { tMin = t; obj = &sphere[0]; }
	t = intersectSphere(&sphere[1], dir, perPixelVars); if (t > 0 && t < tMin) { tMin = t; obj = &sphere[1]; }
	t = intersectSphere(&sphere[2], dir, perPixelVars); if (t > 0 && t < tMin) { tMin = t; obj = &sphere[2]; }

	if (tMin < FAR_FI) {
		hit->pos.x = FP_MUL_T(tMin, dir->x);
		hit->pos.y = FP_MUL_T(tMin, dir->y);
		hit->pos.z = FP_MUL_T(tMin, dir->z);

		// Sphere Normal
		hit->normal.x = -FP_MUL_T((hit->pos.x - obj->a), obj->invR);
		hit->normal.y = -FP_MUL_T((hit->pos.y - obj->b), obj->invR);
		hit->normal.z = -FP_MUL_T((hit->pos.z - obj->c), obj->invR);
	} else { // if still FAR_FI, didn't hit any sphere, so do Plane last
		//t = intersectPlane(&plane, dir);
		t = planeYt[y];	// cheat, we don't even call intersectPlane as it's a very trivial parallel to ground plane, prestored t hit per scanline
		if (t > 0) {
			obj = &plane;

			hit->pos.x = FP_MUL_T(t, dir->x);
			hit->pos.y = FP_MUL_T(t, dir->y);
			hit->pos.z = FP_MUL_T(t, dir->z);

			// Plane Normal
			//hit->normal.x = obj->a;
			//hit->normal.y = obj->b;
			//hit->normal.z = obj->c;
		}
	}
	hit->obj = obj;
}

static void setObjectPlane(Object* obj, Vec3fp pos, Vec3fp dir1, Vec3fp dir2)
{
	obj->pos = pos;
	obj->dir1 = dir1;
	obj->dir2 = dir2;
	obj->type = OBJ_PLANE;
	obj->index = 0;
}

static void setObjectSphere(Object* obj, Vec3fp pos, int radius, int index)
{
	obj->pos = pos;
	obj->radius = radius;
	obj->type = OBJ_SPHERE;
	obj->index = index;
	obj->invR = FP_DIV_T(FLT_TO_FP(1.0f), obj->radius);
}

static void setupObjects()
{
	int i;

	nothing.type = OBJ_NOTHING;

	setObjectPlane(&plane, VEC_FLT_TO_FP(0, 0, -1), VEC_FLT_TO_FP(1, 0, 0), VEC_FLT_TO_FP(0, 0, 1));
	setObjectSphere(&sphere[0], VEC_FLT_TO_FP(0, -1, 4), FLT_TO_FP(0.25f), 1);
	setObjectSphere(&sphere[1], VEC_FLT_TO_FP(-1, -1, 4), FLT_TO_FP(0.25f), 2);
	setObjectSphere(&sphere[2], VEC_FLT_TO_FP(1, -1, 4), FLT_TO_FP(0.25f), 3);

	calcABCD(&plane);
	for (i = 0; i < 3; ++i) {
		calcABCD(&sphere[i]);
	}
}

static void updateObjects(int ticks)
{
	int i;
	for (i = 0; i < 3; ++i) {
		float t = (float)(((i + 1) << 10) + ticks);
		float posX = (float)sin(t / 650.0f) * 0.5f;
		float posY = (float)sin(t / 480.0f) * 0.25f;
		float posZ = 1.25f + (float)sin(t / 880.0f) * 0.5f;

		sphere[i].pos = VEC_FLT_TO_FP(posX, posY, posZ);
		calcABCD(&sphere[i]);
	}

	planeTicks = ticks << 2;
}

static void initVgaPalLUT()
{
	setPalGradient(0,63, 0,0,0, 31,31,31, fbVgaPal);
	setPalGradient(64,127, 0,0,0, 31,15,7, fbVgaPal);
	setPalGradient(128,191, 0,0,0, 7,31,15, fbVgaPal);
	setPalGradient(192,255, 0,0,0, 7,15,31, fbVgaPal);
}

void raytraceInit()
{
	initSQRTLut();

	initVgaPalLUT();

	setupViewDirPerPixelLUTs();

	setupObjects();

	setupPlaneYprecs();

	lightDir0 = VEC_FLT_TO_FP(-0.25f, -0.5f, 0.875f);
	Vnormalize(&lightDir0);
}

static uint16 computeColor(Vec3fp* dir, Hit* hit)
{
	Object* obj = hit->obj;
	int ci = 0;
	int u, v;

	if (obj) {
		switch (obj->type) {
		case OBJ_PLANE:
		{
			//ci = Vdot(dir, &hit->normal);
			ci = FP_MUL_T(dir->y, obj->b);	// hack for speed as plane parallel to ground happens to have Y vector up and everything else is zero.

			CLAMP(ci, 0, ((1 << RP_BITS) - 1));
			u = (hit->pos.x >> COL_BITS_OBJ) & COLS_OBJ_RANGE;
			v = ((hit->pos.z + planeTicks) >> COL_BITS_OBJ) & COLS_OBJ_RANGE;
			ci = FP_MUL((u ^ v), ci);
		}
		break;

		case OBJ_SPHERE:
		{
			ci = Vdot(&lightDir0, &hit->normal) >> (RP_BITS - COL_BITS_OBJ);
			CLAMP(ci, 0, COLS_OBJ_RANGE);
			ci += (obj->index << COL_BITS_OBJ);
		}
		break;
		}
	}
	return fbVgaPal[ci];
}

static uint16 computeColorPlane(Vec3fp* dir, Hit* hit)
{
	Object* obj = hit->obj;
	int u, v;

	//Vec3fp normal;
	//normal.x = obj->a;
	//normal.y = obj->b;
	//normal.z = obj->c;

	//ci = Vdot(dir, &normal);
	int ci = FP_MUL_T(dir->y, obj->b);	// hack for speed as plane parallel to ground happens to have Y vector up and everything else is zero.

	CLAMP(ci, 0, ((1 << RP_BITS) - 1));
	u = (hit->pos.x >> COL_BITS_OBJ) & COLS_OBJ_RANGE;
	v = ((hit->pos.z + planeTicks) >> COL_BITS_OBJ) & COLS_OBJ_RANGE;
	ci = FP_MUL((u ^ v), ci);
	//ci += (obj->index << COL_BITS_OBJ);

	return fbVgaPal[ci];
}

static uint16 computeColorSphere(Hit* hit)
{
	int ci = Vdot(&lightDir0, &hit->normal) >> (RP_BITS - COL_BITS_OBJ);
	CLAMP(ci, 0, COLS_OBJ_RANGE);
	ci += (hit->obj->index << COL_BITS_OBJ);

	return fbVgaPal[ci];
}

static void handleRender2x2(Hit* hitBuff, Vec3fp* dir, PerPixelVars* perPixelVars, int y, uint16* dst)
{
	Hit* hitUL = &hitBuff[0];
	Hit* hitUR = &hitBuff[1];
	Hit* hitLL = &hitBuff[HIT_BUFF_WIDTH];
	Hit* hitLR = &hitBuff[HIT_BUFF_WIDTH + 1];

	Hit hitP;
	if (hitUL->obj == hitUR->obj && hitLL->obj == hitLR->obj && hitUL->obj == hitLL->obj) {
		const int objType = hitUL->obj->type;

		if (objType == OBJ_NOTHING) {
			uint32 *dst32 = (uint32*)dst;
			*dst32 = 0;
			*(dst32 + RT_WIDTH/2) = 0;
		} else {
			const int posULx = hitUL->pos.x;
			const int posULy = hitUL->pos.y;
			const int posULz = hitUL->pos.z;
			Vec3fp* posUR = &hitUR->pos;
			Vec3fp* posLL = &hitLL->pos;
			Vec3fp* posLR = &hitLR->pos;

			*dst = computeColor(dir, hitUL);
			hitP.obj = hitUL->obj;

			if (objType == OBJ_PLANE) {
				hitP.pos.x = (posULx + posUR->x) >> 1; hitP.pos.y = (posULy + posUR->y) >> 1; hitP.pos.z = (posULz + posUR->z) >> 1;
				*(dst + 1) = computeColorPlane(dir + 1, &hitP);
				hitP.pos.x = (posULx + posLL->x) >> 1; hitP.pos.y = (posULy + posLL->y) >> 1; hitP.pos.z = (posULz + posLL->z) >> 1;
				*(dst + RT_WIDTH) = computeColorPlane(dir + RT_BUFF_WIDTH, &hitP);
				hitP.pos.x = (posULx + posLR->x) >> 1; hitP.pos.y = (posULy + posLR->y) >> 1; hitP.pos.z = (posULz + posLR->z) >> 1;
				*(dst + RT_WIDTH + 1) = computeColorPlane(dir + RT_BUFF_WIDTH + 1, &hitP);
			} else {
				const int normULx = hitUL->normal.x;
				const int normULy = hitUL->normal.y;
				const int normULz = hitUL->normal.z;
				Vec3fp* normUR = &hitUR->normal;
				Vec3fp* normLL = &hitLL->normal;
				Vec3fp* normLR = &hitLR->normal;

				hitP.pos.x = (posULx + posUR->x) >> 1; hitP.pos.y = (posULy + posUR->y) >> 1; hitP.pos.z = (posULz + posUR->z) >> 1;
				hitP.normal.x = (normULx + normUR->x) >> 1; hitP.normal.y = (normULy + normUR->y) >> 1; hitP.normal.z = (normULz + normUR->z) >> 1;
				*(dst + 1) = computeColorSphere(&hitP);
				hitP.pos.x = (posULx + posLL->x) >> 1; hitP.pos.y = (posULy + posLL->y) >> 1; hitP.pos.z = (posULz + posLL->z) >> 1;
				hitP.normal.x = (normULx + normLL->x) >> 1; hitP.normal.y = (normULy + normLL->y) >> 1; hitP.normal.z = (normULz + normLL->z) >> 1;
				*(dst + RT_WIDTH) = computeColorSphere(&hitP);
				hitP.pos.x = (posULx + posLR->x) >> 1; hitP.pos.y = (posULy + posLR->y) >> 1; hitP.pos.z = (posULz + posLR->z) >> 1;
				hitP.normal.x = (normULx + normLR->x) >> 1; hitP.normal.y = (normULy + normLR->y) >> 1; hitP.normal.z = (normULz + normLR->z) >> 1;
				*(dst + RT_WIDTH + 1) = computeColorSphere(&hitP);
			}
		}
	} else {
		*dst = computeColor(dir, hitUL);
		traceRay(dir + 1, perPixelVars + 1, &hitP, y); *(dst + 1) = computeColor(dir, &hitP);
		traceRay(dir + RT_BUFF_WIDTH, perPixelVars + RT_BUFF_WIDTH, &hitP, y + 1); *(dst + RT_WIDTH) = computeColor(dir + RT_BUFF_WIDTH, &hitP);
		traceRay(dir + RT_BUFF_WIDTH + 1, perPixelVars + RT_BUFF_WIDTH + 1, &hitP, y + 1); *(dst + RT_WIDTH + 1) = computeColor(dir + RT_BUFF_WIDTH + 1, &hitP);
	}
}

static void renderSubdiv4x(uint16 *buff, int updatePieceIndex)
{
	int x, y;

	const int updatePieceHeight = RT_HEIGHT / RT_UPDATE_PIECES;
	const int y0 = updatePieceIndex * updatePieceHeight;
	const int y1 = y0 + updatePieceHeight;

	Vec3fp* dir = &perPixelPrecs.viewDir[y0 * RT_BUFF_WIDTH];
	PerPixelVars* perPixelVars = &perPixelPrecs.vars[y0 * RT_BUFF_WIDTH];
	Hit *hitBuff = &hitBuffer[(y0 >> 1) * HIT_BUFF_WIDTH];

	uint16* dst = &buff[y0 * RT_WIDTH];

	for (y = y0; y < y1; y+=4) {
		for (x = 0; x < RT_WIDTH; x += 4) {
			Hit* hit0_0 = &hitBuff[0 * HIT_BUFF_WIDTH + 0];
			Hit* hit1_0 = &hitBuff[0 * HIT_BUFF_WIDTH + 1];
			Hit* hit2_0 = &hitBuff[0 * HIT_BUFF_WIDTH + 2];
			Hit* hit0_1 = &hitBuff[1 * HIT_BUFF_WIDTH + 0];
			Hit* hit1_1 = &hitBuff[1 * HIT_BUFF_WIDTH + 1];
			Hit* hit2_1 = &hitBuff[1 * HIT_BUFF_WIDTH + 2];
			Hit* hit0_2 = &hitBuff[2 * HIT_BUFF_WIDTH + 0];
			Hit* hit1_2 = &hitBuff[2 * HIT_BUFF_WIDTH + 1];
			Hit* hit2_2 = &hitBuff[2 * HIT_BUFF_WIDTH + 2];

			Hit hitP;
			hitP.obj = hit0_0->obj;
			if (hit0_0->obj == hit2_0->obj && hit0_2->obj == hit2_2->obj && hit0_0->obj == hit0_2->obj) {
				const int objType = hit0_0->obj->type;

				if (objType == OBJ_NOTHING) {
					uint32 *dst32 = (uint32*)dst;
					*dst32 = 0; *(dst32+1) = 0;
					*(dst32 + RT_WIDTH/2) = 0; *(dst32 + RT_WIDTH/2 + 1) = 0; 
					*(dst32 + RT_WIDTH) = 0; *(dst32 + RT_WIDTH + 1) = 0; 
					*(dst32 + (3*RT_WIDTH)/2) = 0; *(dst32 + (3*RT_WIDTH)/2 + 1) = 0; 
				}
				else {
					*dst = computeColor(dir, hit0_0);
					hitP.obj = hit0_0->obj;

					if (objType == OBJ_PLANE) {
						if (!hit1_0->obj) {
							hit1_0->pos.x = (hit0_0->pos.x + hit2_0->pos.x) >> 1; hit1_0->pos.y = (hit0_0->pos.y + hit2_0->pos.y) >> 1; hit1_0->pos.z = (hit0_0->pos.z + hit2_0->pos.z) >> 1;
							hit1_0->obj = hitP.obj;
						}
						*(dst + 0 * RT_WIDTH + 2) = computeColorPlane(dir + 0 * RT_BUFF_WIDTH + 2, hit1_0);

						if (!hit0_1->obj) {
							hit0_1->pos.x = (hit0_0->pos.x + hit0_2->pos.x) >> 1; hit0_1->pos.y = (hit0_0->pos.y + hit0_2->pos.y) >> 1; hit0_1->pos.z = (hit0_0->pos.z + hit0_2->pos.z) >> 1;
							hit0_1->obj = hitP.obj;
						}
						*(dst + 2 * RT_WIDTH + 0) = computeColorPlane(dir + 2 * RT_BUFF_WIDTH + 0, hit0_1);

						hit1_1->pos.x = (hit0_0->pos.x + hit2_2->pos.x) >> 1; hit1_1->pos.y = (hit0_0->pos.y + hit2_2->pos.y) >> 1; hit1_1->pos.z = (hit0_0->pos.z + hit2_2->pos.z) >> 1;
						hit1_1->obj = hitP.obj;
						*(dst + 2 * RT_WIDTH + 2) = computeColorPlane(dir + 2 * RT_BUFF_WIDTH + 2, hit1_1);

						hit2_1->pos.x = (hit2_0->pos.x + hit2_2->pos.x) >> 1; hit2_1->pos.y = (hit2_0->pos.y + hit2_2->pos.y) >> 1; hit2_1->pos.z = (hit2_0->pos.z + hit2_2->pos.z) >> 1;
						hit2_1->obj = hitP.obj;

						hit1_2->pos.x = (hit0_2->pos.x + hit2_2->pos.x) >> 1; hit1_2->pos.y = (hit0_2->pos.y + hit2_2->pos.y) >> 1; hit1_2->pos.z = (hit0_2->pos.z + hit2_2->pos.z) >> 1;
						hit1_2->obj = hitP.obj;

						hitP.pos.x = (hit0_0->pos.x + hit1_0->pos.x) >> 1; hitP.pos.y = (hit0_0->pos.y + hit1_0->pos.y) >> 1; hitP.pos.z = (hit0_0->pos.z + hit1_0->pos.z) >> 1;
						*(dst + 0 * RT_WIDTH + 1) = computeColorPlane(dir + 0 * RT_BUFF_WIDTH + 1, &hitP);

						hitP.pos.x = (hit1_0->pos.x + hit2_0->pos.x) >> 1; hitP.pos.y = (hit1_0->pos.y + hit2_0->pos.y) >> 1; hitP.pos.z = (hit1_0->pos.z + hit2_0->pos.z) >> 1;
						*(dst + 0 * RT_WIDTH + 3) = computeColorPlane(dir + 0 * RT_BUFF_WIDTH + 3, &hitP);

						hitP.pos.x = (hit0_0->pos.x + hit0_1->pos.x) >> 1; hitP.pos.y = (hit0_0->pos.y + hit0_1->pos.y) >> 1; hitP.pos.z = (hit0_0->pos.z + hit0_1->pos.z) >> 1;
						*(dst + 1 * RT_WIDTH + 0) = computeColorPlane(dir + 1 * RT_BUFF_WIDTH + 0, &hitP);

						hitP.pos.x = (hit0_0->pos.x + hit1_1->pos.x) >> 1; hitP.pos.y = (hit0_0->pos.y + hit1_1->pos.y) >> 1; hitP.pos.z = (hit0_0->pos.z + hit1_1->pos.z) >> 1;
						*(dst + 1 * RT_WIDTH + 1) = computeColorPlane(dir + 1 * RT_BUFF_WIDTH + 1, &hitP);

						hitP.pos.x = (hit1_0->pos.x + hit1_1->pos.x) >> 1; hitP.pos.y = (hit1_0->pos.y + hit1_1->pos.y) >> 1; hitP.pos.z = (hit1_0->pos.z + hit1_1->pos.z) >> 1;
						*(dst + 1 * RT_WIDTH + 2) = computeColorPlane(dir + 1 * RT_BUFF_WIDTH + 2, &hitP);

						hitP.pos.x = (hit1_0->pos.x + hit2_1->pos.x) >> 1; hitP.pos.y = (hit1_0->pos.y + hit2_1->pos.y) >> 1; hitP.pos.z = (hit1_0->pos.z + hit2_1->pos.z) >> 1;
						*(dst + 1 * RT_WIDTH + 3) = computeColorPlane(dir + 1 * RT_BUFF_WIDTH + 3, &hitP);

						hitP.pos.x = (hit0_1->pos.x + hit1_1->pos.x) >> 1; hitP.pos.y = (hit0_1->pos.y + hit1_1->pos.y) >> 1; hitP.pos.z = (hit0_1->pos.z + hit1_1->pos.z) >> 1;
						*(dst + 2 * RT_WIDTH + 1) = computeColorPlane(dir + 2 * RT_BUFF_WIDTH + 1, &hitP);

						hitP.pos.x = (hit1_1->pos.x + hit2_1->pos.x) >> 1; hitP.pos.y = (hit1_1->pos.y + hit2_1->pos.y) >> 1; hitP.pos.z = (hit1_1->pos.z + hit2_1->pos.z) >> 1;
						*(dst + 2 * RT_WIDTH + 3) = computeColorPlane(dir + 2 * RT_BUFF_WIDTH + 3, &hitP);

						hitP.pos.x = (hit0_1->pos.x + hit0_2->pos.x) >> 1; hitP.pos.y = (hit0_1->pos.y + hit0_2->pos.y) >> 1; hitP.pos.z = (hit0_1->pos.z + hit0_2->pos.z) >> 1;
						*(dst + 3 * RT_WIDTH + 0) = computeColorPlane(dir + 3 * RT_BUFF_WIDTH + 0, &hitP);

						hitP.pos.x = (hit0_1->pos.x + hit1_2->pos.x) >> 1; hitP.pos.y = (hit0_1->pos.y + hit1_2->pos.y) >> 1; hitP.pos.z = (hit0_1->pos.z + hit1_2->pos.z) >> 1;
						*(dst + 3 * RT_WIDTH + 1) = computeColorPlane(dir + 3 * RT_BUFF_WIDTH + 1, &hitP);

						hitP.pos.x = (hit1_1->pos.x + hit1_2->pos.x) >> 1; hitP.pos.y = (hit1_1->pos.y + hit1_2->pos.y) >> 1; hitP.pos.z = (hit1_1->pos.z + hit1_2->pos.z) >> 1;
						*(dst + 3 * RT_WIDTH + 2) = computeColorPlane(dir + 3 * RT_BUFF_WIDTH + 2, &hitP);

						hitP.pos.x = (hit1_1->pos.x + hit2_2->pos.x) >> 1; hitP.pos.y = (hit1_1->pos.y + hit2_2->pos.y) >> 1; hitP.pos.z = (hit1_1->pos.z + hit2_2->pos.z) >> 1;
						*(dst + 3 * RT_WIDTH + 3) = computeColorPlane(dir + 3 * RT_BUFF_WIDTH + 3, &hitP);

					}
					else {
						if (!hit1_0->obj) {
							hit1_0->pos.x = (hit0_0->pos.x + hit2_0->pos.x) >> 1; hit1_0->pos.y = (hit0_0->pos.y + hit2_0->pos.y) >> 1; hit1_0->pos.z = (hit0_0->pos.z + hit2_0->pos.z) >> 1;
							hit1_0->normal.x = (hit0_0->normal.x + hit2_0->normal.x) >> 1; hit1_0->normal.y = (hit0_0->normal.y + hit2_0->normal.y) >> 1; hit1_0->normal.z = (hit0_0->normal.z + hit2_0->normal.z) >> 1;
							hit1_0->obj = hitP.obj;
						}
						*(dst + 0 * RT_WIDTH + 2) = computeColorSphere(hit1_0);

						if (!hit0_1->obj) {
							hit0_1->pos.x = (hit0_0->pos.x + hit0_2->pos.x) >> 1; hit0_1->pos.y = (hit0_0->pos.y + hit0_2->pos.y) >> 1; hit0_1->pos.z = (hit0_0->pos.z + hit0_2->pos.z) >> 1;
							hit0_1->normal.x = (hit0_0->normal.x + hit0_2->normal.x) >> 1; hit0_1->normal.y = (hit0_0->normal.y + hit0_2->normal.y) >> 1; hit0_1->normal.z = (hit0_0->normal.z + hit0_2->normal.z) >> 1;
							hit0_1->obj = hitP.obj;
						}
						*(dst + 2 * RT_WIDTH + 0) = computeColorSphere(hit0_1);

						hit1_1->pos.x = (hit0_0->pos.x + hit2_2->pos.x) >> 1; hit1_1->pos.y = (hit0_0->pos.y + hit2_2->pos.y) >> 1; hit1_1->pos.z = (hit0_0->pos.z + hit2_2->pos.z) >> 1;
						hit1_1->normal.x = (hit0_0->normal.x + hit2_2->normal.x) >> 1; hit1_1->normal.y = (hit0_0->normal.y + hit2_2->normal.y) >> 1; hit1_1->normal.z = (hit0_0->normal.z + hit2_2->normal.z) >> 1;
						hit1_1->obj = hitP.obj;
						*(dst + 2 * RT_WIDTH + 2) = computeColorSphere(hit1_1);

						hit2_1->pos.x = (hit2_0->pos.x + hit2_2->pos.x) >> 1; hit2_1->pos.y = (hit2_0->pos.y + hit2_2->pos.y) >> 1; hit2_1->pos.z = (hit2_0->pos.z + hit2_2->pos.z) >> 1;
						hit2_1->normal.x = (hit2_0->normal.x + hit2_2->normal.x) >> 1; hit2_1->normal.y = (hit2_0->normal.y + hit2_2->normal.y) >> 1; hit2_1->normal.z = (hit2_0->normal.z + hit2_2->normal.z) >> 1;
						hit2_1->obj = hitP.obj;

						hit1_2->pos.x = (hit0_2->pos.x + hit2_2->pos.x) >> 1; hit1_2->pos.y = (hit0_2->pos.y + hit2_2->pos.y) >> 1; hit1_2->pos.z = (hit0_2->pos.z + hit2_2->pos.z) >> 1;
						hit1_2->normal.x = (hit0_2->normal.x + hit2_2->normal.x) >> 1; hit1_2->normal.y = (hit0_2->normal.y + hit2_2->normal.y) >> 1; hit1_2->normal.z = (hit0_2->normal.z + hit2_2->normal.z) >> 1;
						hit1_2->obj = hitP.obj;

						hitP.pos.x = (hit0_0->pos.x + hit1_0->pos.x) >> 1; hitP.pos.y = (hit0_0->pos.y + hit1_0->pos.y) >> 1; hitP.pos.z = (hit0_0->pos.z + hit1_0->pos.z) >> 1;
						hitP.normal.x = (hit0_0->normal.x + hit1_0->normal.x) >> 1; hitP.normal.y = (hit0_0->normal.y + hit1_0->normal.y) >> 1; hitP.normal.z = (hit0_0->normal.z + hit1_0->normal.z) >> 1;
						*(dst + 0 * RT_WIDTH + 1) = computeColorSphere(&hitP);

						hitP.pos.x = (hit1_0->pos.x + hit2_0->pos.x) >> 1; hitP.pos.y = (hit1_0->pos.y + hit2_0->pos.y) >> 1; hitP.pos.z = (hit1_0->pos.z + hit2_0->pos.z) >> 1;
						hitP.normal.x = (hit1_0->normal.x + hit2_0->normal.x) >> 1; hitP.normal.y = (hit1_0->normal.y + hit2_0->normal.y) >> 1; hitP.normal.z = (hit1_0->normal.z + hit2_0->normal.z) >> 1;
						*(dst + 0 * RT_WIDTH + 3) = computeColorSphere(&hitP);

						hitP.pos.x = (hit0_0->pos.x + hit0_1->pos.x) >> 1; hitP.pos.y = (hit0_0->pos.y + hit0_1->pos.y) >> 1; hitP.pos.z = (hit0_0->pos.z + hit0_1->pos.z) >> 1;
						hitP.normal.x = (hit0_0->normal.x + hit0_1->normal.x) >> 1; hitP.normal.y = (hit0_0->normal.y + hit0_1->normal.y) >> 1; hitP.normal.z = (hit0_0->normal.z + hit0_1->normal.z) >> 1;
						*(dst + 1 * RT_WIDTH + 0) = computeColorSphere(&hitP);

						hitP.pos.x = (hit0_0->pos.x + hit1_1->pos.x) >> 1; hitP.pos.y = (hit0_0->pos.y + hit1_1->pos.y) >> 1; hitP.pos.z = (hit0_0->pos.z + hit1_1->pos.z) >> 1;
						hitP.normal.x = (hit0_0->normal.x + hit1_1->normal.x) >> 1; hitP.normal.y = (hit0_0->normal.y + hit1_1->normal.y) >> 1; hitP.normal.z = (hit0_0->normal.z + hit1_1->normal.z) >> 1;
						*(dst + 1 * RT_WIDTH + 1) = computeColorSphere(&hitP);

						hitP.pos.x = (hit1_0->pos.x + hit1_1->pos.x) >> 1; hitP.pos.y = (hit1_0->pos.y + hit1_1->pos.y) >> 1; hitP.pos.z = (hit1_0->pos.z + hit1_1->pos.z) >> 1;
						hitP.normal.x = (hit1_0->normal.x + hit1_1->normal.x) >> 1; hitP.normal.y = (hit1_0->normal.y + hit1_1->normal.y) >> 1; hitP.normal.z = (hit1_0->normal.z + hit1_1->normal.z) >> 1;
						*(dst + 1 * RT_WIDTH + 2) = computeColorSphere(&hitP);

						hitP.pos.x = (hit1_0->pos.x + hit2_1->pos.x) >> 1; hitP.pos.y = (hit1_0->pos.y + hit2_1->pos.y) >> 1; hitP.pos.z = (hit1_0->pos.z + hit2_1->pos.z) >> 1;
						hitP.normal.x = (hit1_0->normal.x + hit2_1->normal.x) >> 1; hitP.normal.y = (hit1_0->normal.y + hit2_1->normal.y) >> 1; hitP.normal.z = (hit1_0->normal.z + hit2_1->normal.z) >> 1;
						*(dst + 1 * RT_WIDTH + 3) = computeColorSphere(&hitP);

						hitP.pos.x = (hit0_1->pos.x + hit1_1->pos.x) >> 1; hitP.pos.y = (hit0_1->pos.y + hit1_1->pos.y) >> 1; hitP.pos.z = (hit0_1->pos.z + hit1_1->pos.z) >> 1;
						hitP.normal.x = (hit0_1->normal.x + hit1_1->normal.x) >> 1; hitP.normal.y = (hit0_1->normal.y + hit1_1->normal.y) >> 1; hitP.normal.z = (hit0_1->normal.z + hit1_1->normal.z) >> 1;
						*(dst + 2 * RT_WIDTH + 1) = computeColorSphere(&hitP);

						hitP.pos.x = (hit1_1->pos.x + hit2_1->pos.x) >> 1; hitP.pos.y = (hit1_1->pos.y + hit2_1->pos.y) >> 1; hitP.pos.z = (hit1_1->pos.z + hit2_1->pos.z) >> 1;
						hitP.normal.x = (hit1_1->normal.x + hit2_1->normal.x) >> 1; hitP.normal.y = (hit1_1->normal.y + hit2_1->normal.y) >> 1; hitP.normal.z = (hit1_1->normal.z + hit2_1->normal.z) >> 1;
						*(dst + 2 * RT_WIDTH + 3) = computeColorSphere(&hitP);

						hitP.pos.x = (hit0_1->pos.x + hit0_2->pos.x) >> 1; hitP.pos.y = (hit0_1->pos.y + hit0_2->pos.y) >> 1; hitP.pos.z = (hit0_1->pos.z + hit0_2->pos.z) >> 1;
						hitP.normal.x = (hit0_1->normal.x + hit0_2->normal.x) >> 1; hitP.normal.y = (hit0_1->normal.y + hit0_2->normal.y) >> 1; hitP.normal.z = (hit0_1->normal.z + hit0_2->normal.z) >> 1;
						*(dst + 3 * RT_WIDTH + 0) = computeColorSphere(&hitP);

						hitP.pos.x = (hit0_1->pos.x + hit1_2->pos.x) >> 1; hitP.pos.y = (hit0_1->pos.y + hit1_2->pos.y) >> 1; hitP.pos.z = (hit0_1->pos.z + hit1_2->pos.z) >> 1;
						hitP.normal.x = (hit0_1->normal.x + hit1_2->normal.x) >> 1; hitP.normal.y = (hit0_1->normal.y + hit1_2->normal.y) >> 1; hitP.normal.z = (hit0_1->normal.z + hit1_2->normal.z) >> 1;
						*(dst + 3 * RT_WIDTH + 1) = computeColorSphere(&hitP);

						hitP.pos.x = (hit1_1->pos.x + hit1_2->pos.x) >> 1; hitP.pos.y = (hit1_1->pos.y + hit1_2->pos.y) >> 1; hitP.pos.z = (hit1_1->pos.z + hit1_2->pos.z) >> 1;
						hitP.normal.x = (hit1_1->normal.x + hit1_2->normal.x) >> 1; hitP.normal.y = (hit1_1->normal.y + hit1_2->normal.y) >> 1; hitP.normal.z = (hit1_1->normal.z + hit1_2->normal.z) >> 1;
						*(dst + 3 * RT_WIDTH + 2) = computeColorSphere(&hitP);

						hitP.pos.x = (hit1_1->pos.x + hit2_2->pos.x) >> 1; hitP.pos.y = (hit1_1->pos.y + hit2_2->pos.y) >> 1; hitP.pos.z = (hit1_1->pos.z + hit2_2->pos.z) >> 1;
						hitP.normal.x = (hit1_1->normal.x + hit2_2->normal.x) >> 1; hitP.normal.y = (hit1_1->normal.y + hit2_2->normal.y) >> 1; hitP.normal.z = (hit1_1->normal.z + hit2_2->normal.z) >> 1;
						*(dst + 3 * RT_WIDTH + 3) = computeColorSphere(&hitP);

					}
				}
			} else {
				if (!hit1_0->obj) traceRay(dir + 2, perPixelVars + 2, hit1_0, y);
				if (!hit0_1->obj) traceRay(dir + 2 * RT_BUFF_WIDTH, perPixelVars + 2 * RT_BUFF_WIDTH, hit0_1, y + 2);
				if (!hit1_2->obj) traceRay(dir + 4 * RT_BUFF_WIDTH + 2, perPixelVars + 4 * RT_BUFF_WIDTH + 2, hit1_2, y + 4);
				if (!hit2_1->obj) traceRay(dir + 2 * RT_BUFF_WIDTH + 4, perPixelVars + 2 * RT_BUFF_WIDTH + 4, hit2_1, y + 2);
				if (!hit1_1->obj) traceRay(dir + 2 * RT_BUFF_WIDTH + 2, perPixelVars + 2 * RT_BUFF_WIDTH + 2, hit1_1, y + 2);

				handleRender2x2(hit0_0, dir, perPixelVars, y, dst);
				handleRender2x2(hit1_0, dir + 2, perPixelVars + 2, y, dst + 2);
				handleRender2x2(hit0_1, dir + 2 * RT_BUFF_WIDTH, perPixelVars + 2 * RT_BUFF_WIDTH, y + 2, dst + 2 * RT_WIDTH);
				handleRender2x2(hit1_1, dir + 2 * RT_BUFF_WIDTH + 2, perPixelVars + 2 * RT_BUFF_WIDTH + 2, y + 2, dst + 2 * RT_WIDTH + 2);
			}
			perPixelVars += 4;
			dir += 4;
			dst += 4;
			hitBuff += 2;
		}
		perPixelVars += 3 * RT_BUFF_WIDTH + 1;
		dir += 3 * RT_BUFF_WIDTH + 1;
		dst += 3 * RT_WIDTH;
		hitBuff += 1 * HIT_BUFF_WIDTH + 1;
	}
}

static void renderObjectsSubdiv4x(uint16 *buff, int updatePieceIndex)
{
	int x, y, i;

	const int updatePieceHeight = RT_HEIGHT / RT_UPDATE_PIECES;	// RT_BUFF_HEIGHT happens to be nice number + 1. So we divide RT_HEIGHT instead, maybe handle the last extra line later.
	const int y0 = updatePieceIndex * updatePieceHeight;
	const int y1 = y0 + updatePieceHeight + 1;

	Vec3fp* dir = &perPixelPrecs.viewDir[y0 * RT_BUFF_WIDTH];
	PerPixelVars* perPixelVars = &perPixelPrecs.vars[y0 * RT_BUFF_WIDTH];
	Hit *hitBuff = &hitBuffer[(y0 >> 1) * HIT_BUFF_WIDTH];

	i = 0;
	for (y = y0; y < y1; y+=2) {
		for (x = 0; x < HIT_BUFF_WIDTH; x++) {
			hitBuff[i++].obj = NULL;
		}
	}
		
	for (y = y0; y < y1; y+=4) {
		for (x = 0; x < RT_BUFF_WIDTH; x+=4) {
			if (hitBuff->obj == NULL) {
				traceRay(dir, perPixelVars, hitBuff, y);
			}

			perPixelVars+=4;
			dir += 4;
			hitBuff+=2;
		}
		perPixelVars += 3 * RT_BUFF_WIDTH - 3;
		dir += 3 * RT_BUFF_WIDTH - 3;
		hitBuff +=  HIT_BUFF_WIDTH - 1;
	}

	renderSubdiv4x(buff, updatePieceIndex);
}

void raytraceRun(uint16 *buff, int updatePieceIndex, int ticks)
{
	updateObjects(ticks);

	renderObjectsSubdiv4x(buff, updatePieceIndex);
}
