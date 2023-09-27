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

static void copyVertex(Vertex *srcVertex)
{
	Vertex *v = currentVertex++;

	v->x = srcVertex->x;
	v->y = srcVertex->y;
	v->z = srcVertex->z;
}

static void insertMiddleVertex(Vertex *v0, Vertex *v1)
{
	Vertex *v = currentVertex++;

	v->x = (v0->x + v1->x) >> 1;
	v->y = (v0->y + v1->y) >> 1;
	v->z = (v0->z + v1->z) >> 1;
}

static void insertAverageQuadVertex(Vertex *v0, Vertex *v1, Vertex *v2, Vertex *v3)
{
	Vertex *v = currentVertex++;

	v->x = (v0->x + v1->x + v2->x + v3->x) >> 2;
	v->y = (v0->y + v1->y + v2->y + v3->y) >> 2;
	v->z = (v0->z + v1->z + v2->z + v3->z) >> 2;
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

static void generateWireframe(Mesh *ms)
{
	int i,j,n;
	int *index = ms->index;
	int *lineIndex = ms->lineIndex;
	int lineIndexLast = 0;

	for (i = 0; i < ms->polysNum; ++i) {
		const int numPoints = ms->poly[i].numPoints;
		for (n=0; n<numPoints; ++n) {
			const int i0 = index[n];
			const int i1 = index[(n+1) % numPoints];

			bool found = false;
			for (j = 0; j < lineIndexLast; ++j) {
				if ( (i0==lineIndex[j] && i1==lineIndex[j+1]) || (i1==lineIndex[j] && i0==lineIndex[j+1]) ) {
					found = true;
					break;
				}
			}
			if (!found) {
				addLineIndices(i0, i1);
				lineIndexLast += 2;
			}
		}
		index += numPoints;
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

	static ConvergingNormals convNormalsSet[256];

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
}

void calculateMeshNormals(Mesh *ms)
{
	resetCurrentPolyNormal(ms);
	resetCurrentVertexNormal(ms);
	calculatePolyNormals(ms);
	if (ms->renderType & MESH_OPTION_RENDER_SOFT) calculateVertexNormals(ms);
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

Mesh *subdivMesh(Mesh *srcMesh)
{
	Mesh *dstMesh;
	
	int *srcIndex = srcMesh->index;
	Vertex *srcVertex = srcMesh->vertex;
	PolyData *srcPoly = srcMesh->poly;

	int i;
	int newVertNum = 0;
	for (i=0; i<srcMesh->polysNum; ++i) {
		if (srcPoly[i].numPoints==3) {
			newVertNum += 6;
		} else {
			newVertNum += 9;
		}
	}

	dstMesh = initMesh(getElementsSize(newVertNum, 4*srcMesh->polysNum, 4*srcMesh->indicesNum, 0), srcMesh->renderType, srcMesh->tex);

	resetAllCurrentPointers(dstMesh);

	{
		PolyData *dstPoly = dstMesh->poly;
		int u,v;
		int b = 0;
		for (i=0; i<srcMesh->polysNum; ++i) {
			const int numPoints = srcPoly->numPoints;

			Vertex *v0 = &srcVertex[*srcIndex++];
			Vertex *v1 = &srcVertex[*srcIndex++];
			Vertex *v2 = &srcVertex[*srcIndex++];

			copyVertex(v0);
			copyVertex(v1);
			copyVertex(v2);

			if (numPoints==3) {
				insertMiddleVertex(v0, v1);
				insertMiddleVertex(v1, v2);
				insertMiddleVertex(v2, v0);
				addTriangleIndices(b,  b+3,b+5);
				addTriangleIndices(b+3,b+1,b+4);
				addTriangleIndices(b+3,b+4,b+5);
				addTriangleIndices(b+5,b+4,b+2);
				b += 6;
			} else {
				Vertex *v3 = &srcVertex[*srcIndex++];
				copyVertex(v3);
				insertMiddleVertex(v0, v1);
				insertMiddleVertex(v1, v2);
				insertMiddleVertex(v2, v3);
				insertMiddleVertex(v3, v0);
				insertAverageQuadVertex(v0, v1, v2, v3);
				addQuadIndices(b,  b+4,b+8,b+7);
				addQuadIndices(b+4,b+1,b+5,b+8);
				addQuadIndices(b+7,b+8,b+6,b+3);
				addQuadIndices(b+8,b+5,b+2,b+6);
				b += 9;
			}

			for (v=0; v<2; ++v) {
				for (u=0; u<2; ++u) {
					dstPoly->numPoints = numPoints;
					dstPoly->textureId = srcPoly->textureId;
					dstPoly->palId = 0;

					dstPoly->subtexWidth = srcPoly->subtexWidth / 2;
					dstPoly->subtexHeight = srcPoly->subtexHeight / 2;
					dstPoly->offsetU = srcPoly->offsetU + u * dstPoly->subtexWidth;
					dstPoly->offsetV = srcPoly->offsetV + v * dstPoly->subtexHeight;

					++dstPoly;
				}
			}
			++srcPoly;
		}
	}
	calculateMeshNormals(dstMesh);

	prepareCelList(dstMesh);

	updateMeshCELs(dstMesh);

	destroyMesh(srcMesh);

	return dstMesh;
}

Mesh *initGenMesh(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex)
{
	int i, x, y;
	int xp, yp;
	int dx, dy;

	Mesh *ms = NULL;

	const int size = params.size;
	const int s = size / 2;
	const int n = 1 << NORMAL_SHIFT;
	const int m = (int)(n * 0.57735026919f);	// sqrt(3)/3

	switch(meshgenId)
	{
		default:
		case MESH_PLANE:
		{
			ms = initMesh(getElementsSize(4,1,4,0), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			addVertex(-s, -s, 0);
			addVertex( s, -s, 0);
			addVertex( s,  s, 0);
			addVertex(-s,  s, 0);

			//addQuadIndices(0,1,2,3);
			addQuadIndices(3,2,1,0);

			setAllPolyData(ms,4,0,0);

			prepareCelList(ms);
		}
		break;

		case MESH_CUBE:
		{
			ms = initMesh(getElementsSize(8,6,24,12), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			initCubeVertices(s);
			initCubePolyNormals(n);
			if (ms->renderType & MESH_OPTION_RENDER_SOFT) initCubeVertexNormals(m);

			addQuadIndices(0,1,2,3);
			addQuadIndices(1,4,7,2);
			addQuadIndices(4,5,6,7);
			addQuadIndices(5,0,3,6);
			addQuadIndices(3,2,7,6);
			addQuadIndices(5,4,1,0);

			initCubeLineIndices();

			setAllPolyData(ms,4,0,0);

			prepareCelList(ms);
		}
		break;

		case MESH_CUBE_TRI:
		{
			ms = initMesh(getElementsSize(8,12,36,12), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			initCubeVertices(s);
			initCubePolyNormalsTri(n);
			if (ms->renderType & MESH_OPTION_RENDER_SOFT) initCubeVertexNormals(m);

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

			prepareCelList(ms);
		}
		break;
		
		case MESH_ROMBUS:
		{
			ms = initMesh(getElementsSize(6,8,24,0), optionsFlags, tex);

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

			calculateMeshNormals(ms);

			prepareCelList(ms);
		}
		break;

		case MESH_PRISM:
		{
			ms = initMesh(getElementsSize(6,5,18,0), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			addVertex(-s,  0, -s);
			addVertex( s,  0, -s);
			addVertex( s,  0,  s);
			addVertex(-s,  0,  s);
			addVertex( 0,  s, -s);
			addVertex( 0,  s,  s);

			addQuadIndices(3,2,1,0);	// CW?
			addQuadIndices(1,2,5,4);
			addQuadIndices(3,0,4,5);
			addTriangleIndices(0,1,4);
			addTriangleIndices(2,3,5);

			setAllPolyData(ms,4,0,0);
			ms->poly[3].numPoints = ms->poly[4].numPoints = 3;

			calculateMeshNormals(ms);

			prepareCelList(ms);
		}
		break;

		case MESH_PYRAMID1:
		{
			ms = initMesh(getElementsSize(5,5,20,0), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			initMeshPyramids_1or3(s);

			setAllPolyData(ms,4,0,0);

			prepareCelList(ms);
		}
		break;

		case MESH_PYRAMID2:
		{
			ms = initMesh(getElementsSize(9,5,20,0), optionsFlags, tex);

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
			updatePolyTexData(ms);

			prepareCelList(ms);
		}
		break;

		case MESH_PYRAMID3:
		{
			ms = initMesh(getElementsSize(5,5,20,0), optionsFlags, tex);

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
			updatePolyTexData(ms);

			prepareCelList(ms);
		}
		break;

		case MESH_GRID:
		{
			const int divisions = params.divisions;
			const int vertexNum = (divisions + 1) * (divisions + 1);
			const int polysNum = divisions * divisions;
			const int indicesNum = polysNum * 4;

			ms = initMesh(getElementsSize(vertexNum, polysNum, indicesNum, 0), optionsFlags, tex);

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
					addVertex(xp, 0, -yp);
					//addVertex(xp, ((x*x)^(y*y)) & 15, -yp);
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

			prepareCelList(ms);
		}
		break;

		case MESH_SQUARE_COLUMNOID:
		{
			const int procPointsNum = params.numProcPoints;
			Point2D *procPoints = params.procPoints;

			const int vertexNum = procPointsNum * 4;
			const int polysNum = (procPointsNum-1) * 4 + (int)params.capTop + (int)params.capBottom;
			const int indicesNum = polysNum * 4;
			const int linesNum = (procPointsNum - 1) * 4 + procPointsNum * 4;

			ms = initMesh(getElementsSize(vertexNum, polysNum, indicesNum, linesNum), optionsFlags, tex);

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

			calculateMeshNormals(ms);

			generateWireframe(ms);

			prepareCelList(ms);
		}
		break;

		case MESH_SKYBOX:
		{
			ms = initMesh(getElementsSize(8,6,24,12), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			initCubeVertices(s);
			initCubePolyNormals(n);

			addQuadIndices(3,2,1,0);
			addQuadIndices(2,7,4,1);
			addQuadIndices(7,6,5,4);
			addQuadIndices(6,3,0,5);
			addQuadIndices(6,7,2,3);
			addQuadIndices(0,1,4,5);

			initCubeLineIndices();

			for (i=0; i<ms->polysNum; i++) {
				ms->poly[i].numPoints = 4;
				ms->poly[i].textureId = i;
				ms->poly[i].palId = 0;
			}
			updatePolyTexData(ms);

			prepareCelList(ms);

			for (i=0; i<params.divisions; ++i) {
				ms = subdivMesh(ms);
			}
		}
		break;

		case MESH_STARS:
		{
			const int starsNum = params.numProcPoints;
			const int distance = params.size;

			ms = initMesh(getElementsSize(starsNum,0,0,0), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			for (i=0; i<starsNum; ++i) {
				const int phi = (int)((acos(1 - 2 * ((float)getRand(0, 4096) / 4096.0f)) / PI) * 127);
				const int theta = getRand(0, 255);
				const int sinPhi = SinF16(phi<<16) >> FP_BASE_TO_CORE;
				const int cosPhi = CosF16(phi<<16) >> FP_BASE_TO_CORE;
				const int sinTheta = SinF16(theta<<16) >> FP_BASE_TO_CORE;
				const int cosTheta = CosF16(theta<<16) >> FP_BASE_TO_CORE;
				const int x = (distance * FIXED_MUL(sinPhi, cosTheta, FP_BASE)) >> FP_BASE;
				const int z = (distance * FIXED_MUL(sinPhi, sinTheta, FP_BASE)) >> FP_BASE;
				const int y = (distance * cosPhi) >> FP_BASE;

				addVertex(x, y, z);
			}

			prepareCelList(ms);
		}
		break;

		case MESH_PARTICLES:
		{
			const int particlesNum = params.numProcPoints;
			const int halfSize = 256;

			ms = initMesh(getElementsSize(particlesNum,0,0,0), optionsFlags, tex);

			resetAllCurrentPointers(ms);

			for (i=0; i<particlesNum; ++i) {
				const int x = getRand(-halfSize, halfSize);
				const int y = getRand(0, 2 * halfSize);
				const int z = getRand(-halfSize, halfSize);
				addVertex(x, y, z);
			}

			prepareCelList(ms);
		}
		break;

		case MESH_VOLUME_SLICES:
		{
		}
		break;
	}

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
	params.procPoints = NULL;

	return params;
}

MeshgenParams makeMeshgenSkyboxParams(int size, int subdivisions)
{
	MeshgenParams params;

	params.size = size;
	params.divisions = subdivisions;

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

MeshgenParams makeMeshgenStarsParams(int distance, int numStars)
{
	MeshgenParams params;

	params.size = distance;
	params.numProcPoints = numStars;

	return params;
}

MeshgenParams makeMeshgenParticlesParams(int numParticles)
{
	MeshgenParams params;

	params.numProcPoints = numParticles;

	return params;
}
