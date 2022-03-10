#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_mesh.h"
#include "mathutil.h"

#include "engine_main.h"


static Vertex *currentVertex;
static int *currentIndex;
static int *currentLineIndex;
static Vector3D *currentPolyNormal;
static Vector3D *currentVertexNormal;

typedef struct ConvergingNormals
{
	Vector3D normals[8];
	int num;
}ConvergingNormals;

int fuck[256];


static void setCurrentVertex(Vertex *v)
{
	currentVertex = v;
}

static void setCurrentIndex(int *index)
{
	currentIndex = index;
}

static void setCurrentLineIndex(int *lineIndex)
{
	currentLineIndex = lineIndex;
}

static void setCurrentPolyNormal(Vector3D *polyNormal)
{
	currentPolyNormal = polyNormal;
}

static void setCurrentVertexNormal(Vector3D *vertexNormal)
{
	currentVertexNormal = vertexNormal;
}

static void resetCurrentVertex(Mesh *ms)
{
	setCurrentVertex(ms->vertex);
}

static void resetCurrentIndex(Mesh *ms)
{
	setCurrentIndex(ms->index);
}

static void resetCurrentLineIndex(Mesh *ms)
{
	setCurrentLineIndex(ms->lineIndex);
}

static void resetCurrentPolyNormal(Mesh *ms)
{
	setCurrentPolyNormal(ms->polyNormal);
}

static void resetCurrentVertexNormal(Mesh *ms)
{
	setCurrentVertexNormal(ms->vertexNormal);
}

static void resetAllCurrentPointers(Mesh *ms)
{
	resetCurrentVertex(ms);
	resetCurrentIndex(ms);
	resetCurrentLineIndex(ms);
	resetCurrentPolyNormal(ms);
	resetCurrentVertexNormal(ms);
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

static void addLineIndices(int i0, int i1)
{
	*currentLineIndex++ = i0;
	*currentLineIndex++ = i1;
}

static void addPolyNormal(int x, int y, int z)
{
	Vector3D *normal = currentPolyNormal++;

	normal->x = x;
	normal->y = y;
	normal->z = z;
}

static void addVertexNormal(int x, int y, int z)
{
	Vector3D *normal = currentVertexNormal++;

	normal->x = x;
	normal->y = y;
	normal->z = z;
}

static void setAllPolyData(Mesh *ms, int numPoints, int textureId, int palId)
{
	PolyData *poly = ms->poly;

	int i;
	for (i=0; i<ms->polysNum; i++) {
		poly->numPoints = numPoints;
		poly->textureId = textureId;
		poly->palId = palId;
		++poly;
	}
}


static void calculatePolyNormals(Mesh *ms)
{
	int i;
	int *index = ms->index;
	Vertex *pt0, *pt1, *pt2;
	Vector3D v0, v1, vCross;

	for (i=0; i<ms->polysNum; ++i) {
		pt0 = &ms->vertex[*index++];
		pt1 = &ms->vertex[*index++];
		pt2 = &ms->vertex[*index++];
		if (ms->poly[i].numPoints == 4) ++index;

		setVector3DfromVertices(&v0, pt0, pt1);
		setVector3DfromVertices(&v1, pt2, pt1);

		calcVector3Dcross(&vCross, &v0, &v1);
		normalizeVector3D(&vCross);

		//fuck[3*i] = vCross.x;
		//fuck[3*i+1] = vCross.y;
		//fuck[3*i+2] = vCross.z;

		addPolyNormal(vCross.x, vCross.y, vCross.z);
	}
}

static bool doesVectorAlreadyExistInArrayOfVectors(Vector3D *myVector, Vector3D *vectors, int numVectors)
{
	int i;
	for (i=0; i<numVectors; ++i) {
		if (myVector->x == vectors[i].x && myVector->y == vectors[i].y && myVector->z == vectors[i].z) return true;
	}
	return false;
}

static void calculateVertexNormals(Mesh *ms)
{
	int i,j;
	int *index = ms->index;

	//ConvergingNormals *convNormalsSet = (ConvergingNormals*)AllocMem(ms->verticesNum * sizeof(ConvergingNormals), MEMTYPE_TRACKSIZE);
	static ConvergingNormals convNormalsSet[64];

	for (i=0; i<ms->verticesNum; ++i) {
		convNormalsSet[i].num = 0;
	}

	for (i=0; i<ms->polysNum; ++i) {
		Vector3D *polyNormal = &ms->polyNormal[i];
		
		for (j=0; j<ms->poly[i].numPoints; ++j) {
			ConvergingNormals *convNormals = &convNormalsSet[*index++];
			if (!doesVectorAlreadyExistInArrayOfVectors(polyNormal, convNormals->normals, convNormals->num)) {
				convNormals->normals[convNormals->num].x = polyNormal->x;
				convNormals->normals[convNormals->num].y = polyNormal->y;
				convNormals->normals[convNormals->num].z = polyNormal->z;
				convNormals->num++;
			}
		}
	}

	for (i=0; i<ms->verticesNum; ++i) {
		Vector3D *normals = convNormalsSet[i].normals;
		int num = convNormalsSet[i].num;

		Vector3D normalSum;

		setVector3D(&normalSum, 0,0,0);
		for (j=0; j<num; ++j) {
			normalSum.x += normals[j].x;
			normalSum.y += normals[j].y;
			normalSum.z += normals[j].z;
		}
		normalSum.x /= num;
		normalSum.y /= num;
		normalSum.z /= num;
		addVertexNormal(normalSum.x, normalSum.y, normalSum.z);
	}

	//FreeMem(convNormalsSet, -1);
}

static void calculateNormals(Mesh *ms)
{
	calculatePolyNormals(ms);
	calculateVertexNormals(ms);
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

static void initCubePolyNormals(int n)
{
	addPolyNormal( 0,  0, -n);
	addPolyNormal( n,  0,  0);
	addPolyNormal( 0,  0,  n);
	addPolyNormal(-n,  0,  0);
	addPolyNormal( 0,  n,  0);
	addPolyNormal( 0, -n,  0);
}

static void initCubePolyNormalsTri(int n)
{
	addPolyNormal( 0,  0, -n);
	addPolyNormal( 0,  0, -n);
	addPolyNormal( n,  0,  0);
	addPolyNormal( n,  0,  0);
	addPolyNormal( 0,  0,  n);
	addPolyNormal( 0,  0,  n);
	addPolyNormal(-n,  0,  0);
	addPolyNormal(-n,  0,  0);
	addPolyNormal( 0,  n,  0);
	addPolyNormal( 0,  n,  0);
	addPolyNormal( 0, -n,  0);
	addPolyNormal( 0, -n,  0);
}

static void initCubeVertexNormals(int n)
{
	addVertexNormal(-n, -n, -n);
	addVertexNormal( n, -n, -n);
	addVertexNormal( n,  n, -n);
	addVertexNormal(-n,  n, -n);
	addVertexNormal( n, -n,  n);
	addVertexNormal(-n, -n,  n);
	addVertexNormal(-n,  n,  n);
	addVertexNormal( n,  n,  n);
}

static void initCubeLineIndices()
{
	addLineIndices(0,1);
	addLineIndices(1,2);
	addLineIndices(2,3);
	addLineIndices(3,0);
	addLineIndices(4,5);
	addLineIndices(5,6);
	addLineIndices(6,7);
	addLineIndices(7,4);
	addLineIndices(1,4);
	addLineIndices(0,5);
	addLineIndices(3,6);
	addLineIndices(2,7);
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
	const int n = 1 << NORMAL_SHIFT;
	const int m = (int)(n * 0.57735026919f);	// sqrt(3)/3

	switch(meshgenId)
	{
		default:
		case MESH_PLANE:
		{
			ms = initMesh(4,1,4,0, optionsFlags);

			resetAllCurrentPointers(ms);

			addVertex(-s, -s, 0);
			addVertex( s, -s, 0);
			addVertex( s,  s, 0);
			addVertex(-s,  s, 0);

			addQuadIndices(0,1,2,3);

			setAllPolyData(ms,4,0,0);
		}
		break;

		case MESH_CUBE:
		{
			ms = initMesh(8,6,24,12, optionsFlags);

			resetAllCurrentPointers(ms);

			initCubeVertices(s);
			initCubePolyNormals(n);
			initCubeVertexNormals(m);

			addQuadIndices(0,1,2,3);
			addQuadIndices(1,4,7,2);
			addQuadIndices(4,5,6,7);
			addQuadIndices(5,0,3,6);
			addQuadIndices(3,2,7,6);
			addQuadIndices(5,4,1,0);

			initCubeLineIndices();

			setAllPolyData(ms,4,0,0);
		}
		break;

		case MESH_CUBE_TRI:
		{
			ms = initMesh(8,12,36,12, optionsFlags);

			resetAllCurrentPointers(ms);

			initCubeVertices(s);
			initCubePolyNormalsTri(n);
			initCubeVertexNormals(m);

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

			initCubeLineIndices();

			setAllPolyData(ms,3,0,0);
		}
		break;
		
		case MESH_ROMBUS:
		{
			ms = initMesh(6,8,24,0, optionsFlags);

			resetAllCurrentPointers(ms);

			addVertex(-s,  0, -s);
			addVertex( s,  0, -s);
			addVertex( s,  0,  s);
			addVertex(-s,  0,  s);
			addVertex( 0, -s,  0);
			addVertex( 0,  s,  0);

			addTriangleIndices(0,1,4);
			addTriangleIndices(1,2,4);
			addTriangleIndices(2,3,4);
			addTriangleIndices(3,0,4);
			addTriangleIndices(5,1,0);
			addTriangleIndices(5,2,1);
			addTriangleIndices(5,3,2);
			addTriangleIndices(5,0,3);

			setAllPolyData(ms,3,0,0);

			calculateNormals(ms);
		}
		break;

		case MESH_PYRAMID1:
		{
			ms = initMesh(5,5,20,0, optionsFlags);

			resetAllCurrentPointers(ms);

			initMeshPyramids_1or3(s);

			setAllPolyData(ms,4,0,0);
		}
		break;

		case MESH_PYRAMID2:
		{
			ms = initMesh(9,5,20,0, optionsFlags);

			resetAllCurrentPointers(ms);

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

			for (i=0; i<ms->polysNum; i++) {
				ms->poly[i].numPoints = 4;

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
			ms = initMesh(5,5,20,0, optionsFlags);

			resetAllCurrentPointers(ms);

			initMeshPyramids_1or3(s);

			for (i=0; i<ms->polysNum; i++) {
				ms->poly[i].numPoints = 4;
				ms->poly[i].palId = 0;

				if (i==0) {
					ms->poly[i].textureId = 0;
				} else {
					ms->poly[i].textureId = 1;
				}
			}
		}
		break;

		case MESH_GRID:
		{
			const int divisions = params.divisions;
			const int vertexNum = (divisions + 1) * (divisions + 1);
			const int polysNum = divisions * divisions;
			const int indicesNum = polysNum * 4;

			ms = initMesh(vertexNum, polysNum, indicesNum, 0, optionsFlags);

			resetAllCurrentPointers(ms);

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

			setAllPolyData(ms,4,0,0);
		}
		break;
		
		case MESH_SQUARE_COLUMNOID:
		{
			const int procPointsNum = params.numProcPoints;
			Point2D *procPoints = params.procPoints;

			const int vertexNum = procPointsNum * 4;
			const int polysNum = (procPointsNum-1) * 4 + (int)params.capTop + (int)params.capBottom;
			const int indicesNum = polysNum * 4;

			ms = initMesh(vertexNum, polysNum, indicesNum, 0, optionsFlags);

			resetAllCurrentPointers(ms);

			for (i=0; i<procPointsNum; ++i) {
				const int r = procPoints->x;
				const int y = procPoints->y;

				addVertex(-r, y, -r);
				addVertex( r, y, -r);
				addVertex( r, y,  r);
				addVertex(-r, y,  r);

				++procPoints;
			}

			for (i=0; i<procPointsNum-1; ++i) {
				const int viOff = i * 4;

				addQuadIndices(viOff + 0, viOff + 4, viOff + 5, viOff + 1);
				addQuadIndices(viOff + 1, viOff + 5, viOff + 6, viOff + 2);
				addQuadIndices(viOff + 2, viOff + 6, viOff + 7, viOff + 3);
				addQuadIndices(viOff + 3, viOff + 7, viOff + 4, viOff + 0);
			}
			if (params.capTop) addQuadIndices(0,1,2,3);
			if (params.capBottom) {
				i = 4 *i;
				addQuadIndices(i+3, i+2, i+1, i);
			}

			setAllPolyData(ms,4,0,0);

			calculateNormals(ms);
		}
		break;
		
		case MESH_VOLUME_SLICES:
		{
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

MeshgenParams makeMeshgenSquareColumnoidParams(int size, Point2D *points, int numPoints, bool capTop, bool capBottom)
{
	MeshgenParams params;

	params.size = size;
	params.procPoints = points;
	params.numProcPoints = numPoints;
	params.capTop = capTop;
	params.capBottom = capBottom;

	return params;
}
