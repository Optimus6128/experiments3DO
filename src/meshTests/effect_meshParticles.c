#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"
#include "file_utils.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"
#include "engine_world.h"
#include "engine_view.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"


#define GRID_SIZE 16
#define NUM_PARTICLES 256
#define PARTICLE_BITS 16
#define PARTICLE_UNIT (1 << PARTICLE_BITS)


typedef struct Particle
{
	Vector3D pos;
	Vector3D vel;
	int life;
}Particle;

static Particle particles[NUM_PARTICLES];

static Viewer *viewer;
static Light *light;

static Mesh *gridMesh;
static Object3D *gridObj;

static Texture *gridTex;
static uint16 gridPal[32];

static Texture *blobTex;
static uint16 blobPal[32];

static Mesh *particlesMesh;
static Object3D *particlesObj;

static World *myWorld;


static void shadeGrid()
{
	int x,y;
	CCB *cel = gridMesh->cel;

	for (y=0; y<GRID_SIZE; ++y) {
		const int yc = y - GRID_SIZE / 2;
		for (x=0; x<GRID_SIZE; ++x) {
			const int xc = x - GRID_SIZE / 2;
			int r = (isqrt(xc*xc + yc*yc) * 16) / (GRID_SIZE / 2);
			CLAMP(r,0,15)
			cel->ccb_PIXC = shadeTable[15-r];
			++cel;
		}
	}
}

static void emitParticle(Particle *particle)
{
	setVector3D(&particle->pos, 0,0,0);
	setVector3D(&particle->vel, getRand(-PARTICLE_UNIT, PARTICLE_UNIT) / 1, 8 * PARTICLE_UNIT,getRand(-PARTICLE_UNIT, PARTICLE_UNIT) / 1);
}

static void initParticles()
{
	int i;
	for (i=0; i<NUM_PARTICLES; ++i) {
		emitParticle(&particles[i]);
		particles[i].life = -i;
	}
}

void effectMeshParticlesInit()
{
	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	MeshgenParams particlesParams = makeMeshgenParticlesParams(NUM_PARTICLES);
	
	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);
	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	gridObj = initObject3D(gridMesh);
	shadeGrid();

	setPalGradient(0,31, 0,0,0, 27,29,31, blobPal);
	blobTex = initGenTexture(8,8, 8, blobPal, 1, TEXGEN_BLOB, NULL);

	particlesMesh = initGenMesh(MESH_PARTICLES, particlesParams, MESH_OPTION_RENDER_BILLBOARDS | MESH_OPTION_NO_POLYSORT, blobTex);
	setMeshTranslucency(particlesMesh, true, true);
	particlesObj = initObject3D(particlesMesh);

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);
	
	addObjectToWorld(gridObj, 0, false, myWorld);
	addObjectToWorld(particlesObj, 1, false, myWorld);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);

	initParticles();
}

static void animateParticles(int dt)
{
	int i;
	const int emits = 2;
	const int gravity = PARTICLE_UNIT >> 3;
	const int count = particlesMesh->verticesNum;
	Vertex *v = particlesMesh->vertex;
	Particle *p = particles;

	for (i=0; i<count; ++i) {
		if (p->life > 0) {
			p->pos.x += ((p->vel.x * dt) >> 4);
			p->pos.y += ((p->vel.y * dt) >> 4);
			p->pos.z += ((p->vel.z * dt) >> 4);
			p->vel.y -= ((gravity * dt) >> 4);
			if (p->pos.y < 0) {
				p->pos.y = 0;
				if (p->vel.y < 0) p->vel.y = -p->vel.y >> 1;
			}
		}

		v->x = p->pos.x >> PARTICLE_BITS;
		v->y = p->pos.y >> PARTICLE_BITS;
		v->z = p->pos.z >> PARTICLE_BITS;
		++v;

		p->life -= emits;
		if (p->life <= -NUM_PARTICLES) {
			p->life += 2*NUM_PARTICLES;
			emitParticle(p);
		}
		++p;
	}
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

void effectMeshParticlesRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	animateParticles(dt);

	renderWorld(myWorld);
}
