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
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
		int i;
		for (i=0; i<ms->quadsNum; i++) {
			Texture *tex = &ms->tex[ms->quad[i].textureId];

			if (tex->type & TEXTURE_TYPE_DYNAMIC) {
				int woffset;
				int vcnt;
				CCB *cel = ms->cel;

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
				cel->ccb_PLUTPtr = (uint16*)&tex->pal[ms->quad[i].palId << getPaletteColorsNum(tex->bpp)];
			}
		}
	}
}

void prepareCelList(Mesh *ms)
{
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
		int i;

		for (i=0; i<ms->quadsNum; i++)
		{
			Texture *tex = &ms->tex[ms->quad[i].textureId];
			CCB *cel = &ms->cel[i];

			int celType = CEL_TYPE_UNCODED;
			if (tex->type & TEXTURE_TYPE_PALLETIZED)
				celType = CEL_TYPE_CODED;

			initCel(tex->width, tex->height, tex->bpp, celType, cel);
			setupCelData((uint16*)&tex->pal[ms->quad[i].palId << getPaletteColorsNum(tex->bpp)], tex->bitmap, cel);

			cel->ccb_PLUTPtr = (uint16*)&tex->pal[ms->quad[i].palId << getPaletteColorsNum(tex->bpp)];

			cel->ccb_Flags &= ~CCB_ACW;	// Initially, ACW is off and only ACCW (counterclockwise) polygons are visible

			cel->ccb_Flags |= CCB_BGND;
			if (!(tex->type & TEXTURE_TYPE_FEEDBACK))
				cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);	// Enable Super Clipping only if Feedback Texture is not enabled, it might lock otherwise

			if (i!=0) LinkCel(&ms->cel[i-1], &ms->cel[i]);
		}
		ms->cel[ms->quadsNum-1].ccb_Flags |= CCB_LAST;
	}
}

static void setMeshCELflags(Mesh *ms, uint32 flags, bool enable)
{
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
		int i;
		CCB *cel = ms->cel;

		for (i=0; i<ms->quadsNum; i++) {
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
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
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
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
		int i;
		CCB *cel = ms->cel;

		for (i=0; i<ms->quadsNum; i++) {
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
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
		setMeshCELflags(ms, CCB_BGND, !enable);
	}
}

void setMeshDottedDisplay(Mesh *ms, bool enable)
{
	if (ms->renderType & MESH_OPTION_RENDER_HARD) {
		setMeshCELflags(ms, CCB_MARIA, enable);
	}
}

Mesh* initMesh(int vrtxNum, int quadsNum, int renderType)
{
	Mesh *ms = (Mesh*)AllocMem(sizeof(Mesh), MEMTYPE_ANY);

	ms->vrtxNum = vrtxNum;
	ms->quadsNum = quadsNum;

	ms->indexNum = ms->quadsNum << 2;
	ms->vrtx = (Vertex*)AllocMem(ms->vrtxNum * sizeof(Vertex), MEMTYPE_ANY);
	ms->index = (int*)AllocMem(ms->indexNum * sizeof(int), MEMTYPE_ANY);
	ms->quad = (QuadData*)AllocMem(ms->quadsNum * sizeof(QuadData), MEMTYPE_ANY);

	if (renderType & MESH_OPTION_RENDER_HARD) {
		ms->cel = (CCB*)AllocMem(ms->quadsNum * sizeof(CCB), MEMTYPE_ANY);
	}
	if (renderType & MESH_OPTION_RENDER_SOFT) {
		ms->indexCol = (uint32*)AllocMem(ms->indexNum * sizeof(uint32), MEMTYPE_ANY);
		ms->indexTC = (TexCoords*)AllocMem(ms->indexNum * sizeof(TexCoords), MEMTYPE_ANY);
	}
	ms->renderType = renderType;

	return ms;
}
