#include "core.h"

#include "tools.h"
#include "cel_helpers.h"
#include "system_graphics.h"

#include "engine_mesh.h"
#include "engine_texture.h"
#include "procgen_mesh.h"
#include "mathutil.h"
#include "file_utils.h"


void updateMeshCELs(Mesh *ms)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		int i;
		for (i=0; i<ms->polysNum; i++) {
			Texture *tex = &ms->tex[ms->poly[i].textureId];
			int woffset;
			int vcnt;
			CCB *cel = &ms->cel[i];

			const int texShrX = getShr(tex->width);
			const int texShrY = getShr(tex->height);
			ms->poly[i].texShifts = (texShrX << 4) | texShrY;

			// In the future, also take account of offscreen buffer position too
			if (tex->type & TEXTURE_TYPE_FEEDBACK) {
				cel->ccb_Flags &= ~(CCB_ACSC | CCB_ALSC);
				cel->ccb_PRE1 |= PRE1_LRFORM;
				cel->ccb_SourcePtr = (CelData*)getBackBufferByIndex(tex->bufferIndex);
				woffset = SCREEN_WIDTH - 2;
				vcnt = (tex->height / 2) - 1;
			} else {
				cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);
				cel->ccb_PRE1 &= ~PRE1_LRFORM;
				cel->ccb_SourcePtr = (CelData*)tex->bitmap;
				woffset = tex->width / 2 - 2;
				vcnt = tex->height - 1;
			}

			// Should spare the magic numbers at some point
			cel->ccb_PRE0 = (cel->ccb_PRE0 & ~(((1<<10) - 1)<<6)) | (vcnt << 6);
			cel->ccb_PRE1 = (cel->ccb_PRE1 & (65536 - 1024)) | (woffset << 16) | (tex->width-1);
			cel->ccb_PLUTPtr = (uint16*)&tex->pal[ms->poly[i].palId << getCelPaletteColorsRealBpp(tex->bpp)];
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
			Texture *tex = ms->tex;
			uint16 *pal = (uint16*)tex->pal;

			CCB *cel = &ms->cel[i];
			int celType = CEL_TYPE_UNCODED;

			if (!isBillBoards) {
				int texShrX, texShrY;

				tex = &ms->tex[ms->poly[i].textureId];
				texShrX = getShr(tex->width);
				texShrY = getShr(tex->height);
				ms->poly[i].texShifts = (texShrX << 4) | texShrY;

				pal = (uint16*)&tex->pal[ms->poly[i].palId << getCelPaletteColorsRealBpp(tex->bpp)];
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

void setMeshTranslucency(Mesh *ms, bool enable, bool additive)
{
	uint32 pixcBlend = TRANSLUCENT_CEL;
	if (additive) pixcBlend = 0x1F801F80;

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
				cel->ccb_PIXC = SOLID_CEL;
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

void setAllPolyData(Mesh *ms, int numPoints, int textureId, int palId)
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

Mesh* initMesh(int verticesNum, int polysNum, int indicesNum, int linesNum, int renderType)
{
	Mesh *ms = (Mesh*)AllocMem(sizeof(Mesh), MEMTYPE_ANY);

	ms->verticesNum = verticesNum;
	ms->polysNum = polysNum;
	ms->indicesNum = indicesNum;
	ms->linesNum = linesNum;

	if (verticesNum) ms->vertex = (Vertex*)AllocMem(verticesNum * sizeof(Vertex), MEMTYPE_ANY);
	if (indicesNum) ms->index = (int*)AllocMem(indicesNum * sizeof(int), MEMTYPE_ANY);
	if (linesNum) ms->lineIndex = (int*)AllocMem(linesNum * 2 * sizeof(int), MEMTYPE_ANY);
	if (polysNum) {
		ms->poly = (PolyData*)AllocMem(polysNum * sizeof(PolyData), MEMTYPE_ANY);
		ms->polyNormal = (Vector3D*)AllocMem(polysNum * sizeof(Vector3D), MEMTYPE_ANY);
	}

	if (renderType & MESH_OPTION_RENDER_SOFT) {
		if (verticesNum) {
			ms->vertexNormal = (Vector3D*)AllocMem(verticesNum * sizeof(Vector3D), MEMTYPE_ANY);
			ms->vertexCol = (int*)AllocMem(verticesNum * sizeof(int), MEMTYPE_ANY);
			ms->vertexTC = (TexCoords*)AllocMem(verticesNum * sizeof(TexCoords), MEMTYPE_ANY);
		}
	} else {
		if (renderType & MESH_OPTION_RENDER_BILLBOARDS) {
			if (verticesNum) {
				ms->cel = (CCB*)AllocMem(verticesNum * sizeof(CCB), MEMTYPE_ANY);
				ms->poly = (PolyData*)AllocMem(verticesNum * sizeof(PolyData), MEMTYPE_ANY);
			}
		} else if (polysNum) {
			ms->cel = (CCB*)AllocMem(polysNum * sizeof(CCB), MEMTYPE_ANY);
		}
	}
	ms->renderType = renderType;

	ms->texturesNum = 0;
	ms->tex = NULL;

	return ms;
}

Mesh *loadMesh(char *path, int loadOptions, int meshOptions, Texture *tex)
{
	int i;
	int verticesNum, polysNum, linesNum, tempBuffSize;

	char *tempBuffSrc;
	uint8 *tempBuff8;
	uint16 *tempBuff16;

	Mesh *ms = NULL;

	bool loadLines = !(loadOptions & MESH_LOAD_SKIP_LINES);
	bool flipPolyOrder = loadOptions & MESH_LOAD_FLIP_POLYORDER;


	openFileStream(path); 

	tempBuffSrc = readSequentialBytesFromFile(6);
	tempBuff16 = (uint16*)tempBuffSrc;

	verticesNum = SHORT_ENDIAN_FLIP(tempBuff16[0]);
	polysNum = SHORT_ENDIAN_FLIP(tempBuff16[2]);
	linesNum = SHORT_ENDIAN_FLIP(tempBuff16[1]);
	tempBuffSize = verticesNum * 3;


	ms = initMesh(verticesNum, polysNum, 3*polysNum, linesNum * (int)loadLines, meshOptions);
	ms->tex = tex;

	tempBuffSrc = readSequentialBytesFromFile(tempBuffSize);
	tempBuff8 = (uint8*)tempBuffSrc;
	for (i=0; i<verticesNum; ++i) {
		ms->vertex[i].x = 127 - *tempBuff8++;
		ms->vertex[i].y = 127 - *tempBuff8++;
		ms->vertex[i].z = 127 - *tempBuff8++;
	}

	tempBuffSize = 2*linesNum * sizeof(uint16);
	if (loadLines) {
		tempBuffSrc = readSequentialBytesFromFile(tempBuffSize);
		tempBuff16 = (uint16*)tempBuffSrc;
		for (i=0; i<2*linesNum; ++i) {
			ms->lineIndex[i] = SHORT_ENDIAN_FLIP(tempBuff16[i]);
		}
	} else {
		moveFilePointerRelative(tempBuffSize);
	}

	tempBuffSize = 3*polysNum * sizeof(uint16);
	tempBuffSrc = readSequentialBytesFromFile(tempBuffSize);
	tempBuff16 = (uint16*)tempBuffSrc;
	for (i=0; i<3*polysNum; ++i) {
		ms->index[i] = SHORT_ENDIAN_FLIP(tempBuff16[i]);
	}
	setAllPolyData(ms, 3, 0, 0);

	if (flipPolyOrder) flipMeshPolyOrder(ms);

	calculateMeshNormals(ms);
	prepareCelList(ms);

	// Commenting out this will make things fail for uknown reasons
	//closeFileStream();
	
	return ms;
}
