#ifndef MAIN_H
#define MAIN_H

void effectMeshPyramidsInit(void);
void effectMeshPyramidsRun(void);

void effectMeshGridInit(void);
void effectMeshGridRun(void);

void effectMeshSoftInit(void);
void effectMeshSoftRun(void);

void effectMeshWorldInit(void);
void effectMeshWorldRun(void);

void effectMeshSkyboxInit(void);
void effectMeshSkyboxRun(void);

void effectMeshLoadInit(void);
void effectMeshLoadRun(void);

void effectMeshParticlesInit(void);
void effectMeshParticlesRun(void);

void effectMeshHeightmapInit(void);
void effectMeshHeightmapRun(void);

void effectMeshFliInit(void);
void effectMeshFliRun(void);


enum { EFFECT_MESH_PYRAMIDS, EFFECT_MESH_GRID, EFFECT_MESH_SOFT, EFFECT_MESH_WORLD, EFFECT_MESH_SKYBOX, EFFECT_MESH_LOAD, EFFECT_MESH_PARTICLES, EFFECT_MESH_HEIGHTMAP, EFFECT_MESH_FLI, EFFECTS_NUM };

void(*effectInitFunc[EFFECTS_NUM])() = { effectMeshPyramidsInit, effectMeshGridInit, effectMeshSoftInit, effectMeshWorldInit, effectMeshSkyboxInit, effectMeshLoadInit, effectMeshParticlesInit, effectMeshHeightmapInit, effectMeshFliInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectMeshPyramidsRun, effectMeshGridRun, effectMeshSoftRun, effectMeshWorldRun, effectMeshSkyboxRun, effectMeshLoadRun, effectMeshParticlesRun, effectMeshHeightmapRun, effectMeshFliRun };

char *effectName[EFFECTS_NUM] = { "mesh pyramids test", "mesh grid", "software 3d", "3d world", "skybox", "mesh load", "particles", "heightmap", "FLI plane" };

#endif
