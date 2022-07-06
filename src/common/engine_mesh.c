#include "core.h"

#include "tools.h"
#include "cel_helpers.h"
#include "system_graphics.h"

#include "engine_mesh.h"
#include "engine_texture.h"
#include "mathutil.h"

static int getPaletteColorsNum(int bpp)
{
	if (bpp <= 4) {
		return bpp;
	}
	if (bpp <= 8) {
		return 5;
	}
	return 0;
}

void updateMeshCELs(Mesh *ms)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		int i;
		for (i=0; i<ms->polysNum; i++) {
			Texture *tex = &ms->tex[ms->poly[i].textureId];
			if (tex->type & TEXTURE_TYPE_DYNAMIC) {
				int woffset;
				int vcnt;
				CCB *cel = ms->cel;

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
				cel->ccb_PLUTPtr = (uint16*)&tex->pal[ms->poly[i].palId << getPaletteColorsNum(tex->bpp)];
			}
		}
	}
}

void prepareCelList(Mesh *ms)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		int i;
		for (i=0; i<ms->polysNum; i++)
		{
			Texture *tex = &ms->tex[ms->poly[i].textureId];
			const int texShrX = getShr(tex->width);
			const int texShrY = getShr(tex->height);
			CCB *cel = &ms->cel[i];

			int celType = CEL_TYPE_UNCODED;
			if (tex->type & TEXTURE_TYPE_PALLETIZED)
				celType = CEL_TYPE_CODED;

			ms->poly[i].texShifts = (texShrX << 4) | texShrY;

			initCel(tex->width, tex->height, tex->bpp, celType, cel);
			setupCelData((uint16*)&tex->pal[ms->poly[i].palId << getPaletteColorsNum(tex->bpp)], tex->bitmap, cel);

			cel->ccb_PLUTPtr = (uint16*)&tex->pal[ms->poly[i].palId << getPaletteColorsNum(tex->bpp)];

			cel->ccb_Flags &= ~CCB_ACW;	// Initially, ACW is off and only ACCW (counterclockwise) polygons are visible

			cel->ccb_Flags |= CCB_BGND;
			if (!(tex->type & TEXTURE_TYPE_FEEDBACK))
				cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);	// Enable Super Clipping only if Feedback Texture is not enabled, it might lock otherwise

			if (i!=0) linkCel(&ms->cel[i-1], &ms->cel[i]);
		}
		ms->cel[ms->polysNum-1].ccb_Flags |= CCB_LAST;
	}
}

void setMeshPaletteIndex(int palIndex, Mesh *ms)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		int i;
		for (i=0; i<ms->polysNum; i++) {
			Texture *tex = &ms->tex[ms->poly[i].textureId];
			if (tex) {
				ms->poly[i].palId = palIndex;
				ms->cel[i].ccb_PLUTPtr = (uint16*)&tex->pal[palIndex << getPaletteColorsNum(tex->bpp)];
			}
		}
	}
}

static void setMeshCELflags(Mesh *ms, uint32 flags, bool enable)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		CCB *cel = ms->cel;
		int i;
		for (i=0; i<ms->polysNum; i++) {
			if (enable) {
				cel->ccb_Flags |= flags;
			} else {
				cel->ccb_Flags &= ~flags;
			}
			++cel;
		}
	}
}

void setMeshPolygonOrder(Mesh *ms, bool cw, bool ccw)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		if (cw) {
			setMeshCELflags(ms, CCB_ACW, true);
		} else {
			setMeshCELflags(ms, CCB_ACW, false);
		}

		if (ccw) {
			setMeshCELflags(ms, CCB_ACCW, true);
		} else {
			setMeshCELflags(ms, CCB_ACCW, false);
		}
	}
}

void setMeshTranslucency(Mesh *ms, bool enable)
{
	if (!(ms->renderType & MESH_OPTION_RENDER_SOFT)) {
		CCB *cel = ms->cel;
		int i;
		for (i=0; i<ms->polysNum; i++) {
			if (enable) {
				cel->ccb_PIXC = TRANSLUCENT_CEL;
			} else {
				cel->ccb_PIXC = SOLID_CEL;
			}
			++cel;
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

Mesh* initMesh(int verticesNum, int polysNum, int indicesNum, int linesNum, int renderType)
{
	Mesh *ms = (Mesh*)AllocMem(sizeof(Mesh), MEMTYPE_ANY);

	ms->verticesNum = verticesNum;
	ms->polysNum = polysNum;
	ms->indicesNum = indicesNum;
	ms->linesNum = linesNum;

	ms->vertex = (Vertex*)AllocMem(ms->verticesNum * sizeof(Vertex), MEMTYPE_ANY);
	ms->index = (int*)AllocMem(ms->indicesNum * sizeof(int), MEMTYPE_ANY);
	ms->poly = (PolyData*)AllocMem(polysNum * sizeof(PolyData), MEMTYPE_ANY);
	ms->lineIndex = (int*)AllocMem(linesNum * 2 * sizeof(int), MEMTYPE_ANY);
	ms->polyNormal = (Vector3D*)AllocMem(ms->polysNum * sizeof(Vector3D), MEMTYPE_ANY);

	if (renderType & MESH_OPTION_RENDER_SOFT) {
		ms->vertexNormal = (Vector3D*)AllocMem(ms->verticesNum * sizeof(Vector3D), MEMTYPE_ANY);
		ms->vertexCol = (int*)AllocMem(ms->verticesNum * sizeof(int), MEMTYPE_ANY);
		ms->vertexTC = (TexCoords*)AllocMem(ms->verticesNum * sizeof(TexCoords), MEMTYPE_ANY);
	} else {
		ms->cel = (CCB*)AllocMem(polysNum * sizeof(CCB), MEMTYPE_ANY);
	}
	ms->renderType = renderType;

	return ms;
}
