#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_mesh.h"
#include "mathutil.h"


static Vertex *currentVertex;
static int *currentIndex;


static void setCurrentVertex(Vertex *v)
{
	currentVertex = v;
}

static void setCurrentIndex(int *index)
{
	currentIndex = index;
}

static void addVertex(int x, int y, int z)
{
	Vertex *v = currentVertex++;

	v->x = x;
	v->y = y;
	v->z = z;
}

static void addQuadIndices(int i0, int i1, int i2, int i3)
{
	*currentIndex++ = i0;
	*currentIndex++ = i1;
	*currentIndex++ = i2;
	*currentIndex++ = i3;
}

static void addTriangleIndices(int i0, int i1, int i2)
{
	*currentIndex++ = i0;
	*currentIndex++ = i1;
	*currentIndex++ = i2;
}

static void setAllPolyData(Mesh *ms, int textureId, int palId)
{
	const int polysNum = ms->quadsNum + ms->trianglesNum;
	int i;

	for (i=0; i<polysNum; i++) {
		ms->poly[i].textureId = textureId;
		ms->poly[i].palId = palId;
	}
}

static void initCubeVertices(int s)
{
	addVertex(-s, -s, -s);
	addVertex( s, -s, -s);
	addVertex( s,  s, -s);
	addVertex(-s,  s, -s);
	addVertex( s, -s,  s);
	addVertex(-s, -s,  s);
	addVertex(-s,  s,  s);
	addVertex( s,  s,  s);
}

static void initMeshPyramids_1or3(int s)
{
	addVertex(-s, -s, -s);
	addVertex( s, -s, -s);
	addVertex( s, -s,  s);
	addVertex(-s, -s,  s);
	addVertex( 0,  s,  0);

	addQuadIndices(3,2,1,0);
	addQuadIndices(0,1,4,4);
	addQuadIndices(1,2,4,4);
	addQuadIndices(2,3,4,4);
	addQuadIndices(3,0,4,4);
}

Mesh *initGenMesh(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex)
{
	int i, x, y;
	int xp, yp;
	int dx, dy;

	Mesh *ms;

	const int size = params.size;
	const int s = size / 2;

	switch(meshgenId)
	{
		default:
		case MESH_PLANE:
		{
			ms = initMesh(4,1,0, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			addVertex(-s, -s, 0);
			addVertex( s, -s, 0);
			addVertex( s,  s, 0);
			addVertex(-s,  s, 0);

			addQuadIndices(0,1,2,3);

			setAllPolyData(ms,0,0);
		}
		break;

		case MESH_CUBE:
		{
			ms = initMesh(8,6,0, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			initCubeVertices(s);

			addQuadIndices(0,1,2,3);
			addQuadIndices(1,4,7,2);
			addQuadIndices(4,5,6,7);
			addQuadIndices(5,0,3,6);
			addQuadIndices(3,2,7,6);
			addQuadIndices(5,4,1,0);

			setAllPolyData(ms,0,0);
		}
		break;

		case MESH_CUBE_TRI:
		{
			ms = initMesh(8,0,12, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			initCubeVertices(s);

			addTriangleIndices(0,1,2);
			addTriangleIndices(0,2,3);
			addTriangleIndices(1,4,7);
			addTriangleIndices(1,7,2);
			addTriangleIndices(4,5,6);
			addTriangleIndices(4,6,7);
			addTriangleIndices(5,0,3);
			addTriangleIndices(5,3,6);
			addTriangleIndices(3,2,7);
			addTriangleIndices(3,7,6);
			addTriangleIndices(5,4,1);
			addTriangleIndices(5,1,0);

			setAllPolyData(ms,0,0);
		}
		break;

		case MESH_PYRAMID1:
		{
			ms = initMesh(5,5,0, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			initMeshPyramids_1or3(s);

			setAllPolyData(ms,0,0);
		}
		break;

		case MESH_PYRAMID2:
		{
			ms = initMesh(9,5,0, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			addVertex(-s, -s, -s);
			addVertex( s, -s, -s);
			addVertex( s, -s,  s);
			addVertex(-s, -s,  s);

			addVertex( 2*s, s, 0);
			addVertex(   0, s, 2*s);
			addVertex(-2*s, s, 0);
			addVertex(   0, s, -2*s);

			addVertex(0, s, 0);


			addQuadIndices(3,2,1,0);
			addQuadIndices(0,1,4,8);
			addQuadIndices(1,2,5,8);
			addQuadIndices(2,3,6,8);
			addQuadIndices(3,0,7,8);

			for (i=0; i<ms->quadsNum; i++) {
				if (i==0) {
					ms->poly[i].textureId = 0;
					ms->poly[i].palId = 0;
				} else {
					ms->poly[i].textureId = 1;
					ms->poly[i].palId = 1;
				}
			}
		}
		break;

		case MESH_PYRAMID3:
		{
			ms = initMesh(5,5,0, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			initMeshPyramids_1or3(s);

			for (i=0; i<ms->quadsNum; i++) {
				if (i==0) {
					ms->poly[i].textureId = 0;
				} else {
					ms->poly[i].textureId = 1;
				}
				ms->poly[i].palId = 0;
			}
		}
		break;

		case MESH_GRID:
		{
			const int divisions = params.divisions;
			const int vrtxNum = (divisions + 1) * (divisions + 1);
			const int quadsNum = divisions * divisions;

			ms = initMesh(vrtxNum, quadsNum, 0, optionsFlags);

			setCurrentVertex(ms->vrtx);
			setCurrentIndex(ms->index);

			dx = size / divisions;
			dy = size / divisions;

			i = 0;
			yp = -size / 2;
			for (y=0; y<=divisions; y++)
			{
				xp = -size / 2;
				for (x=0; x<=divisions; x++)
				{
					addVertex(xp, getRand(0, 255), -yp);
					xp += dx;
					i++;
				}
				yp += dy;
			}

			for (y=0; y<divisions; y++) {
				for (x=0; x<divisions; x++) {
					addQuadIndices(	x + (y + 1) * (divisions + 1), 
									x + 1 + (y + 1) * (divisions + 1), 
									x + 1 + y * (divisions + 1), 
									x + y * (divisions + 1));
				}
			}

			setAllPolyData(ms,0,0);
		}
		break;
	}

	ms->tex = tex;

	prepareCelList(ms);

	return ms;
}

MeshgenParams makeDefaultMeshgenParams(int size)
{
	MeshgenParams params;

	params.size = size;

	return params;
}

MeshgenParams makeMeshgenGridParams(int size, int divisions)
{
	MeshgenParams params;

	params.size = size;
	params.divisions = divisions;

	return params;
}
