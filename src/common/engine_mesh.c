#include "core.h"

#include "tools.h"
#include "cel_helpers.h"
#include "system_graphics.h"

#include "engine_mesh.h"
#include "engine_texture.h"
#include "procgen_mesh.h"
#include "mathutil.h"
#include "file_utils.h"

static void setCelTexShifts(CCB *cel, PolyData *poly)
{
	const int texShrX = getShr(poly->subtexWidth);
	const int texShrY = getShr(poly->subtexHeight);
	poly->texShifts = (texShrX << 4) | texShrY;
}

void updateMeshCELs(Mesh *ms)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		int i;
		for (i=0; i<ms->polysNum; i++) {
			PolyData *poly = &ms->poly[i];
			Texture *tex = &ms->tex[poly->textureId];
			int woffset;
			int vcnt;
			CCB *cel = &ms->cel[i];

			setCelTexShifts(cel, poly);

			// In the future, also take account of offscreen buffer position too
			if (tex->type & TEXTURE_TYPE_FEEDBACK) {
				cel->ccb_Flags &= ~(CCB_ACSC | CCB_ALSC);
				cel->ccb_PRE1 |= PRE1_LRFORM;
				cel->ccb_SourcePtr = (CelData*)getBackBufferByIndex(tex->bufferIndex);
				woffset = SCREEN_WIDTH - 2;
				vcnt = (tex->height / 2) - 1;
			} else {
				const int xPos32 = (poly->offsetU * tex->bpp) / 32;
				const int lineSize32 = (tex->width * tex->bpp) / 32;

				cel->ccb_Flags = (cel->ccb_Flags & ~CCB_ACSC) | CCB_ALSC;
				cel->ccb_PRE1 &= ~PRE1_LRFORM;
				cel->ccb_SourcePtr = (CelData*)&tex->bitmap[4 * (poly->offsetV * lineSize32 + xPos32)];
				woffset = lineSize32 - 2;
				vcnt = poly->subtexHeight - 1;
			}

			// Should spare the magic numbers at some point
			cel->ccb_PRE0 = (cel->ccb_PRE0 & ~(((1<<10) - 1)<<6)) | (vcnt << 6);
			cel->ccb_PRE1 = (cel->ccb_PRE1 & (65536 - 1024)) | (woffset << 16) | (poly->subtexWidth-1);

			// Update the CEL palette too
			if (tex->pal) {
				cel->ccb_PLUTPtr = (uint16*)&tex->pal[poly->palId << getCelPaletteColorsRealBpp(tex->bpp)];
			}
		}
	}
}

void prepareCelList(Mesh *ms)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		const bool isBillBoards = (ms->renderType & MESH_OPTION_RENDER_BILLBOARDS) != 0;
		int i;
		int celsNum = ms->polysNum;
		if (isBillBoards) celsNum = ms->verticesNum;

		for (i=0; i<celsNum; i++) {
			PolyData *poly = &ms->poly[i];
			Texture *tex = &ms->tex[poly->textureId];
			uint16 *pal = (uint16*)tex->pal;

			CCB *cel = &ms->cel[i];
			int celType = CEL_TYPE_UNCODED;

			if (!isBillBoards) {

				setCelTexShifts(cel, poly);

				if (tex->pal) {
					pal = (uint16*)&tex->pal[poly->palId << getCelPaletteColorsRealBpp(tex->bpp)];
				}
			}

			if (tex->type & TEXTURE_TYPE_PALLETIZED) {
				celType = CEL_TYPE_CODED;
			}

			initCel(tex->width, tex->height, tex->bpp, celType, cel);
			setupCelData(pal, tex->bitmap, cel);

			if (!isBillBoards) {
				cel->ccb_Flags &= ~CCB_ACW;	// Initially, ACW is off and only ACCW (counterclockwise) polygons are visible
				cel->ccb_Flags |= CCB_BGND;
			}

			if (!(tex->type & TEXTURE_TYPE_FEEDBACK)) {
				cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);	// Enable Super Clipping only if Feedback Texture is not enabled, it might lock otherwise
			}

			cel->ccb_Flags &= ~CCB_LAST;
		}
	}
}

void setMeshTexture(Mesh *ms, Texture *tex)
{
	ms->tex = tex;
}

void setMeshPaletteIndex(Mesh *ms, int palIndex)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		const bool isBillBoards = (ms->renderType & MESH_OPTION_RENDER_BILLBOARDS) != 0;
		int i;
		int celsNum = ms->polysNum;
		if (isBillBoards) celsNum = ms->verticesNum;

		for (i=0; i<celsNum; i++) {
			Texture *tex = ms->tex;
			if (isBillBoards) {
				tex = &ms->tex[ms->poly[i].textureId];
			} else {
				ms->poly[i].palId = palIndex;
			}
			if (tex) {
				ms->cel[i].ccb_PLUTPtr = (uint16*)&tex->pal[palIndex << getCelPaletteColorsRealBpp(tex->bpp)];
			}
		}
	}
}

static void setMeshCELflags(Mesh *ms, uint32 flags, bool enable)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		const bool isBillBoards = (ms->renderType & MESH_OPTION_RENDER_BILLBOARDS) != 0;
		int i;
		int celsNum = ms->polysNum;
		if (isBillBoards) celsNum = ms->verticesNum;

		for (i=0; i<celsNum; i++) {
			CCB *cel = &ms->cel[i];
			if (enable) {
				cel->ccb_Flags |= flags;
			} else {
				cel->ccb_Flags &= ~flags;
			}
		}
	}
}

void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		setMeshCELflags(ms, CCB_ACW, cw);
		setMeshCELflags(ms, CCB_ACCW, ccw);
	}
}

void setMeshPolygonCPUbackfaceTest(Mesh *ms, bool enable)
{
	if (enable) {
		ms->renderType |= MESH_OPTION_CPU_POLYTEST;
	} else {
		ms->renderType &= ~MESH_OPTION_CPU_POLYTEST;
	}
}

void setMeshTranslucency(Mesh *ms, bool enable, bool additive)
{
	uint32 pixcBlend = CEL_BLEND_AVERAGE;
	if (additive) pixcBlend = CEL_BLEND_ADDITIVE;

	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		const bool isBillBoards = (ms->renderType & MESH_OPTION_RENDER_BILLBOARDS) != 0;
		int i;
		int celsNum = ms->polysNum;
		if (isBillBoards) celsNum = ms->verticesNum;

		for (i=0; i<celsNum; i++) {
			CCB *cel = &ms->cel[i];
			if (enable) {
				cel->ccb_PIXC = pixcBlend;
			} else {
				cel->ccb_PIXC = CEL_BLEND_OPAQUE;
			}
		}
	}
}

void setMeshTransparency(Mesh *ms, bool enable)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		setMeshCELflags(ms, CCB_BGND, !enable);
	}
}

void setMeshDottedDisplay(Mesh *ms, bool enable)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		setMeshCELflags(ms, CCB_MARIA, enable);
	}
}

void updatePolyTexData(Mesh *ms)
{
	int i;
	for (i=0; i<ms->polysNum; i++) {
		PolyData *poly = &ms->poly[i];
		Texture *tex = &ms->tex[poly->textureId];

		poly->offsetU = 0;
		poly->offsetV = 0;
		poly->subtexWidth = tex->width;
		poly->subtexHeight = tex->height;
	}
}

void setAllPolyData(Mesh *ms, int numPoints, int textureId, int palId)
{
	int i;

	for (i=0; i<ms->polysNum; i++) {
		PolyData *poly = &ms->poly[i];
		poly->numPoints = numPoints;
		poly->textureId = textureId;
		poly->palId = palId;
		++poly;
	}
	
	updatePolyTexData(ms);
}

void flipMeshPolyOrder(Mesh *ms)
{
	int i,j;
	int *index = ms->index;
	for (i=0; i<ms->polysNum; ++i) {
		const int numPoints = ms->poly[i].numPoints;

		for (j=0; j<numPoints/2; ++j) {
			const int k = numPoints-1 - j;
			const int tempIndex = *(index + j);
			*(index + j) = *(index + k);
			*(index + k) = tempIndex;
		}
		index += numPoints;
	}
}

void scaleMesh(Mesh *ms, int scaleX, int scaleY, int scaleZ)
{
	int i;
	Vertex *vrtx = ms->vertex;
	for (i=0; i<ms->verticesNum; ++i) {
		vrtx->x *= scaleX;
		vrtx->y *= scaleY;
		vrtx->z *= scaleZ;
		++vrtx;
	}
}

void flipMeshVerticesIfNeg(Mesh *ms, bool flipX, int flipY, bool flipZ)
{
	int i;
	Vertex *vrtx = ms->vertex;
	for (i=0; i<ms->verticesNum; ++i) {
		if (flipX && vrtx->x < 0) vrtx->x = -vrtx->x;
		if (flipY && vrtx->y < 0) vrtx->y = -vrtx->y;
		if (flipZ && vrtx->z < 0) vrtx->z = -vrtx->z;
		++vrtx;
	}
}

ElementsSize *getElementsSize(int verticesNum, int polysNum, int indicesNum, int linesNum)
{
	static ElementsSize elSize;

	elSize.verticesNum = verticesNum;
	elSize.polysNum = polysNum;
	elSize.indicesNum = indicesNum;
	elSize.linesNum = linesNum;

	return &elSize;
}

void destroyMesh(Mesh *ms)
{
	if (ms->vertex) FreeMem(ms->vertex, -1);
	if (ms->index) FreeMem(ms->index, -1);
	if (ms->poly) FreeMem(ms->poly, -1);
	if (ms->polyNormal) FreeMem(ms->polyNormal, -1);
	if (ms->vertexNormal) FreeMem(ms->vertexNormal, -1);
	if (ms->lineIndex) FreeMem(ms->lineIndex, -1);
	if (ms->cel) FreeMem(ms->cel, -1);
	if (ms->vertexCol) FreeMem(ms->vertexCol, -1);
	if (ms->vertexTC) FreeMem(ms->vertexTC, -1);

	ScavengeMem();
}

Mesh* initMesh(ElementsSize *elSize, int renderType, Texture *tex)
{
	Mesh *ms = (Mesh*)AllocMem(sizeof(Mesh), MEMTYPE_ANY);

	ms->vertex = NULL;
	ms->index = NULL;
	ms->poly = NULL;
	ms->polyNormal = NULL;
	ms->vertexNormal = NULL;
	ms->lineIndex = NULL;
	ms->cel = NULL;
	ms->vertexCol = NULL;
	ms->vertexTC = NULL;

	if (elSize) {
		ms->verticesNum = elSize->verticesNum;
		ms->polysNum = elSize->polysNum;
		ms->indicesNum = elSize->indicesNum;
		ms->linesNum = elSize->linesNum;
		ms->renderType = renderType;
		ms->tex = tex;

		if (elSize->verticesNum) ms->vertex = (Vertex*)AllocMem(elSize->verticesNum * sizeof(Vertex), MEMTYPE_TRACKSIZE);
		if (elSize->indicesNum) ms->index = (int*)AllocMem(elSize->indicesNum * sizeof(int), MEMTYPE_TRACKSIZE);
		if (elSize->linesNum) ms->lineIndex = (int*)AllocMem(elSize->linesNum * 2 * sizeof(int), MEMTYPE_TRACKSIZE);
		if (elSize->polysNum) {
			ms->poly = (PolyData*)AllocMem(elSize->polysNum * sizeof(PolyData), MEMTYPE_TRACKSIZE);
			ms->polyNormal = (Vector3D*)AllocMem(elSize->polysNum * sizeof(Vector3D), MEMTYPE_TRACKSIZE);
		}

		if (renderType & MESH_OPTION_RENDER_SOFT) {
			if (elSize->verticesNum) {
				ms->vertexNormal = (Vector3D*)AllocMem(elSize->verticesNum * sizeof(Vector3D), MEMTYPE_TRACKSIZE);
				ms->vertexCol = (int*)AllocMem(elSize->verticesNum * sizeof(int), MEMTYPE_TRACKSIZE);
				ms->vertexTC = (TexCoords*)AllocMem(elSize->verticesNum * sizeof(TexCoords), MEMTYPE_TRACKSIZE);
			}
		} else {
			if (renderType & MESH_OPTION_RENDER_BILLBOARDS) {
				if (elSize->verticesNum) {
					ms->cel = (CCB*)AllocMem(elSize->verticesNum * sizeof(CCB), MEMTYPE_TRACKSIZE);
					ms->poly = (PolyData*)AllocMem(elSize->verticesNum * sizeof(PolyData), MEMTYPE_TRACKSIZE);
				}
			} else if (elSize->polysNum) {
				ms->cel = (CCB*)AllocMem(elSize->polysNum * sizeof(CCB), MEMTYPE_TRACKSIZE);
			}
		}
	}

	return ms;
}

static char tempBuffSrc[16384];	// will alloc later, changed things in file utils and so I forgot about this. Afraid to do AllocMem inside now because compiler issues might emerge again.

Mesh *loadMesh(char *path, int loadOptions, int meshOptions, Texture *tex)
{
	int i;
	unsigned char *tempBuff8;
	uint16 *tempBuff16;
	int tempBuffSize;

	Mesh *ms = NULL;

	bool loadLines = !(loadOptions & MESH_LOAD_SKIP_LINES);
	bool flipPolyOrder = loadOptions & MESH_LOAD_FLIP_POLYORDER;

	Stream *CDstream = openFileStream(path); 
	int verticesNum, polysNum, linesNum, indicesNum;

	readSequentialBytesFromFileStream(6, tempBuffSrc, CDstream);
	tempBuff16 = (uint16*)tempBuffSrc;

	verticesNum = tempBuff16[0];
	linesNum = tempBuff16[1];
	polysNum = tempBuff16[2];

	#ifdef BIG_ENDIAN
		verticesNum = SHORT_ENDIAN_FLIP(verticesNum);
		polysNum = SHORT_ENDIAN_FLIP(polysNum);
		linesNum = SHORT_ENDIAN_FLIP(linesNum);
	#endif

	indicesNum = 3 * polysNum;
	tempBuffSize = verticesNum * 3;


	ms = initMesh(getElementsSize(verticesNum, polysNum, indicesNum, linesNum * (int)loadLines), meshOptions, tex);

	readSequentialBytesFromFileStream(tempBuffSize, tempBuffSrc, CDstream);
	tempBuff8 = (unsigned char*)tempBuffSrc;
	for (i=0; i<verticesNum; ++i) {
		ms->vertex[i].x = 127 - *tempBuff8++;
		ms->vertex[i].y = 127 - *tempBuff8++;
		ms->vertex[i].z = 127 - *tempBuff8++;
	}

	tempBuffSize = 2*linesNum * sizeof(uint16);
	if (loadLines) {
		readSequentialBytesFromFileStream(tempBuffSize, tempBuffSrc, CDstream);
		tempBuff16 = (uint16*)tempBuffSrc;
		for (i=0; i<2*linesNum; ++i) {
			ms->lineIndex[i] = tempBuff16[i];
			#ifdef BIG_ENDIAN
				ms->lineIndex[i] = SHORT_ENDIAN_FLIP(ms->lineIndex[i]);
			#endif
		}
	} else {
		moveFileStreamPointerRelative(tempBuffSize, CDstream);
	}

	tempBuffSize = indicesNum * sizeof(uint16);
	readSequentialBytesFromFileStream(tempBuffSize, tempBuffSrc, CDstream);
	tempBuff16 = (uint16*)tempBuffSrc;
	for (i=0; i<indicesNum; ++i) {
		ms->index[i] = tempBuff16[i];
		#ifdef BIG_ENDIAN
				ms->index[i] = SHORT_ENDIAN_FLIP(ms->index[i]);
		#endif
	}
	setAllPolyData(ms, 3, 0, 0);

	if (flipPolyOrder) flipMeshPolyOrder(ms);

	calculateMeshNormals(ms);
	prepareCelList(ms);

	// Commenting out this will make things fail for uknown reasons (UPDATE: Not this time)
	//closeFileStream(CDstream);

	return ms;
}
