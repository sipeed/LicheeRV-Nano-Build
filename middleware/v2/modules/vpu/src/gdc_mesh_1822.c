#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include <pthread.h>
#include <inttypes.h>

#if !(defined(__SOC_MARS__) || defined(__SOC_PHOBOS__))
#include "cvi_common.h"
#include "cvi_math.h"
#include "cvi_comm_video.h"
#endif

#include "cvi_base.h"
#include "cvi_vpss.h"
#include "cvi_sys.h"
#include "cvi_gdc.h"
#include "gdc_mesh.h"

#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8

#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE

#define QFORMAT_M 10 // mesh coordinate's floating precision bit number.
#define PHASEBIT 8 // pixel interpolation phase
#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8

// HW:
#define MAX_WIDTH_MESH_NUM 16 // offline calibration mesh number
#define MAX_HEIGHT_MESH_NUM 16 // offline calibration mesh number.
#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE
#define MAX_FRAME_MESH_NUM (MAX_WIDTH_MESH_NUM * MAX_HEIGHT_MESH_NUM)

// Fixed CONFIGURATION
#define TWOS_COMPLEMENT 1 // ifndef: sign bit representation:
#define MAX_REGION_NUM 1

#define minmax(a, b, c)  (((a) < (b)) ? (b):((a) > (c)) ? (c) : (a))

// #define SAVE_MESH_TBL_FILE
// #define ENABLE_PROFILE

typedef struct {
	int dwa_en;

	int stage2_rotate_type; // 0: +90, 1: -90
	int bg_color_y_r;
	int bg_color_u_g;
	int bg_color_v_b;

	int dst_x0;
	int dst_y0;

	int src_xstr_s1;
	int src_xend_s1;
	int src_xstr_s2;
	int src_xend_s2;
} _reg_dwa;

typedef struct COORD2D {
	double xcor;
	double ycor;
} COORD2D;

typedef struct COORD2D_INT {
	int xcor;
	int ycor;
} COORD2D_INT;

typedef struct COORD2D_INT_HW {
	CVI_U8 xcor[3]; // s13.10, 24bit
} __attribute__((packed)) COORD2D_INT_HW;

typedef struct MESH_STRUCT {
	COORD2D knot[4];
	int idx;
} MESH_STRUCT;

typedef struct LDC_ATTR {
	int Enable; // dewarp engine on/off

	int RgnNum; // dewarp Region Number.
	int TotalMeshNum; // total mesh numbers
	int OutW_disp; // output display frame width
	int OutH_disp; // output display frame height
	int InCenterX; // input fisheye center position X
	int InCenterY; // input fisheye center position Y
	int InRadius; // input fisheye radius in pixels.

	// frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
	MESH_STRUCT DstRgnMeshInfo[MAX_FRAME_MESH_NUM];

	// frame-based src mesh info.
	MESH_STRUCT SrcRgnMeshInfo[MAX_FRAME_MESH_NUM];

	// frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
	MESH_STRUCT DstRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];

	// frame-based src mesh info.
	MESH_STRUCT SrcRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];

	// frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
	MESH_STRUCT DstRgnMeshInfoExt2ND[9 * MAX_FRAME_MESH_NUM];

	// frame-based src mesh info.
	MESH_STRUCT SrcRgnMeshInfoExt2ND[9 * MAX_FRAME_MESH_NUM];

	// frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
	MESH_STRUCT DstRgnMeshInfo_1st[MAX_FRAME_MESH_NUM];

	// frame-based src mesh info.
	MESH_STRUCT SrcRgnMeshInfo_1st[MAX_FRAME_MESH_NUM];

	// frame-based dst mesh info. maximum num=128*28 = 16384 meshes.
	MESH_STRUCT DstRgnMeshInfo_2nd[MAX_FRAME_MESH_NUM];

	// frame-based src mesh info.
	MESH_STRUCT SrcRgnMeshInfo_2nd[MAX_FRAME_MESH_NUM];

	// How many tile is sliced in a frame horizontally.
	int SliceX_Num;

	// How many tile is sliced in a frame vertically.
	int SliceY_Num;
} LDC_ATTR;

typedef struct LDC_RGN_ATTR {
	int RgnIndex; // region index
	int InRadius; // For Panorama.360 mode, inner radius.
	int OutRadius; // For Panorama.360 mode, outer radius.
	int Pan; // Region PTZ-Pan.
	int HorZoom; // adjust horizontal zoom in Region PTZ.
	int VerZoom; // adjust vertical zoom in Region PTZ.
	int OutX; // region initial position x on display frame.
	int OutY; // region initial position y on display frame.
	int OutW; // region width.
	int OutH; // region height.
	int MeshHor; // mesh counts horizontal
	int MeshVer; // mesh counts vertical

	// to give region default view center
	// User set rotation angle around X-axis, create angle between
	// vector to Z-axis. (start position @ [0,0,1])
	int ThetaX;

	int ThetaZ; // User set rotation angle around Z-axis

	int ZoomV; // User Set for Region
	int PanEnd; // For Panorama Case Only
	int RegionValid; // label valid/ invalid for each region

	// initial buffer to store destination mesh info. max: 32*32
	MESH_STRUCT DstRgnMeshInfo[MAX_FRAME_MESH_NUM];

	// extend to 3x3 range.
	MESH_STRUCT DstRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];

	// initial buffer to store source mesh info
	MESH_STRUCT SrcRgnMeshInfo[MAX_FRAME_MESH_NUM];

	// extend to 3x3 range.
	MESH_STRUCT SrcRgnMeshInfoExt[9 * MAX_FRAME_MESH_NUM];
} LDC_RGN_ATTR;

typedef struct Vector2D {
	double x;
	double y;
} Vector2D;

static int _chk_in_mesh(int x, int y, MESH_STRUCT cur_sw_mesh);
static COORD2D _find_knot_map2src(COORD2D_INT cur_dst_mesh, LDC_ATTR *cfg,
				  int swmesh_hit_index, int stageID);

static int _double2Int_s13_10(double value);

void mesh_gen_get_1st_size(SIZE_S in_size, CVI_U32 *mesh_1st_size)
{
	if (!mesh_1st_size)
		return;

	CVI_U32 ori_src_width = in_size.u32Width;
	CVI_U32 ori_src_height = in_size.u32Height;

	// In LDC Processing, width & height  aligned to TILESIZE **
	CVI_U32 src_width_s1 =
		((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
	CVI_U32 src_height_s1 =
		((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

	// modify frame size
	CVI_U32 dst_height_s1 = src_height_s1;
	CVI_U32 dst_width_s1 = src_width_s1;
	CVI_U32 num_tilex_s1 = dst_width_s1 / TILESIZE;
	CVI_U32 num_tiley_s1 = dst_height_s1 / TILESIZE;

	*mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE *
			 MESH_NUM_ATILE * num_tilex_s1 *
			 num_tiley_s1 * 4; // 4 = 4 knots in a mesh
}

void mesh_gen_get_2nd_size(SIZE_S in_size, CVI_U32 *mesh_2nd_size)
{
	if (!mesh_2nd_size)
		return;

	CVI_U32 ori_src_width = in_size.u32Width;
	CVI_U32 ori_src_height = in_size.u32Height;

	// In LDC Processing, width & height aligned to TILESIZE
	CVI_U32 src_width_s1 =
		((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
	CVI_U32 src_height_s1 =
		((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

	// modify frame size
	CVI_U32 dst_height_s1 = src_height_s1;
	CVI_U32 dst_width_s1 = src_width_s1;
	CVI_U32 src_height_s2 = dst_width_s1;
	CVI_U32 src_width_s2 = dst_height_s1;
	CVI_U32 dst_height_s2 = src_height_s2;
	CVI_U32 dst_width_s2 = src_width_s2;
	CVI_U32 num_tilex_s2 = dst_width_s2 / TILESIZE;
	CVI_U32 num_tiley_s2 = dst_height_s2 / TILESIZE;

	*mesh_2nd_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE *
			 MESH_NUM_ATILE * num_tilex_s2 *
			 num_tiley_s2 * 4; // 4 = 4 knots in a mesh
}

void mesh_gen_get_size(SIZE_S in_size, SIZE_S out_size,
		       CVI_U32 *mesh_1st_size,
		       CVI_U32 *mesh_2nd_size)
{
	if (!mesh_1st_size || !mesh_2nd_size)
		return;

	(void)out_size;

	mesh_gen_get_1st_size(in_size, mesh_1st_size);
	mesh_gen_get_2nd_size(in_size, mesh_2nd_size);
}

#ifdef SAVE_MESH_TBL_FILE
static void _save_1st_src_mdxy(int num_tilex_s1, COORD2D_INT_HW *src_1st_list,
			       int tidx, int tidy, FILE *fp1x)
{
	int midx, midy, xidx, yidx;
	uint32_t idx;

	for (midy = 0; midy < MESH_NUM_ATILE; midy++) {
		for (midx = 0; midx < MESH_NUM_ATILE; midx++) {
			xidx = tidx * MESH_NUM_ATILE + midx;
			yidx = tidy * MESH_NUM_ATILE + midy;
			idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 0;

			fprintf(fp1x, "%02x%02x%02x ",
				src_1st_list[idx].xcor[2],
				src_1st_list[idx].xcor[1],
				src_1st_list[idx].xcor[0]);

			if (midx == MESH_NUM_ATILE - 1) {
				idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 1;
				fprintf(fp1x, "%02x%02x%02x ",
					src_1st_list[idx].xcor[2],
					src_1st_list[idx].xcor[1],
					src_1st_list[idx].xcor[0]);
			}
		}

		if (midy == MESH_NUM_ATILE - 1) {
			for (midx = 0; midx < MESH_NUM_ATILE; midx++) {
				xidx = tidx * MESH_NUM_ATILE + midx;
				yidx = tidy * MESH_NUM_ATILE + midy;
				idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 3;
				fprintf(fp1x, "%02x%02x%02x ",
					src_1st_list[idx].xcor[2],
					src_1st_list[idx].xcor[1],
					src_1st_list[idx].xcor[0]);

				if (midx == MESH_NUM_ATILE - 1) {
					idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 2;
					fprintf(fp1x,
						"%02x%02x%02x ",
						src_1st_list[idx].xcor[2],
						src_1st_list[idx].xcor[1],
						src_1st_list[idx].xcor[0]);
				}
			}
		}
	}
	fprintf(fp1x, "\n");
}

static void save_1st_src_mesh_table(int num_tiley_s1, int num_tilex_s1,
				    COORD2D_INT_HW *src_1st_list)
{
	// packed data in dram for HW
	FILE *fp1x = fopen("srcx_1st_mesh.txt", "w");

	if (!fp1x)
		return;

	for (int tidy = 0; tidy < num_tiley_s1; tidy++) {
		for (int tidx = 0; tidx < num_tilex_s1; tidx++)
			_save_1st_src_mdxy(num_tilex_s1, src_1st_list, tidx, tidy, fp1x);
	}
	fclose(fp1x);
}

static void _save_2nd_src_mdxy(int num_tilex_s2, COORD2D_INT_HW *src_2nd_list,
			       int tidx, int tidy, FILE *fp2x)
{
	int midx, midy, xidx, yidx;
	uint32_t idx;

	for (midy = 0; midy < MESH_NUM_ATILE; midy++) {
		for (midx = 0; midx < MESH_NUM_ATILE; midx++) {
			xidx = tidx * MESH_NUM_ATILE + midx;
			yidx = tidy * MESH_NUM_ATILE + midy;
			idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 0;

			fprintf(fp2x, "%02x%02x%02x ",
				src_2nd_list[idx].xcor[2],
				src_2nd_list[idx].xcor[1],
				src_2nd_list[idx].xcor[0]);
			if (midx == MESH_NUM_ATILE - 1) {
				idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 1;

				fprintf(fp2x, "%02x%02x%02x ",
					src_2nd_list[idx].xcor[2],
					src_2nd_list[idx].xcor[1],
					src_2nd_list[idx].xcor[0]);
			}
		}

		if (midy == MESH_NUM_ATILE - 1) {
			for (midx = 0; midx < MESH_NUM_ATILE; midx++) {
				xidx = tidx * MESH_NUM_ATILE + midx;
				yidx = tidy * MESH_NUM_ATILE + midy;
				idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 3;

				fprintf(fp2x, "%02x%02x%02x ",
					src_2nd_list[idx].xcor[2],
					src_2nd_list[idx].xcor[1],
					src_2nd_list[idx].xcor[0]);

				if (midx == MESH_NUM_ATILE - 1) {
					idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 2;
					fprintf(fp2x,
						"%02x%02x%02x ",
						src_2nd_list[idx].xcor[2],
						src_2nd_list[idx].xcor[1],
						src_2nd_list[idx].xcor[0]);
				}
			}
		}
	}
	fprintf(fp2x, "\n");
}

static void save_2nd_src_mesh_table(int num_tiley_s2, int num_tilex_s2,
				    COORD2D_INT_HW *src_2nd_list,
				    CVI_U32 mesh_2nd_size)
{
	// packed data in dram for HW
	FILE *fp2x_bin = fopen("srcx_2nd_mesh.bin", "wb");

	if (fp2x_bin) {
		size_t wr_size = fwrite(src_2nd_list, mesh_2nd_size, 1, fp2x_bin);

		if (wr_size != mesh_2nd_size)
			CVI_TRACE_GDC(CVI_DBG_ERR,
				"2nd src mesh, fwrite %d, only %d succeed\n",
				mesh_2nd_size, wr_size);
		fclose(fp2x_bin);
	}

	FILE *fp2x = fopen("srcx_2nd_mesh.txt", "w");

	if (fp2x) {
		for (int tidy = 0; tidy < num_tiley_s2; tidy++) {
			for (int tidx = 0; tidx < num_tilex_s2; tidx++)
				_save_2nd_src_mdxy(num_tilex_s2, src_2nd_list, tidx, tidy, fp2x);
		}
		fclose(fp2x);
	}
}

static void save_mesh_info(LDC_ATTR *cfg)
{
	FILE *fpsrc = fopen("mesh_frm_src.txt", "w");
	FILE *fpdst = fopen("mesh_frm_dst.txt", "w");

	for (int meshidx = 0; meshidx < 9 * MAX_FRAME_MESH_NUM; meshidx++) {
		fprintf(fpsrc, "meshidx = %d\n", meshidx);
		fprintf(fpdst, "meshidx = %d\n", meshidx);

		for (int knotidx = 0; knotidx < 4; knotidx++) {
			fprintf(fpsrc, "[%d]( %f, %f)\n", knotidx,
				cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor,
				cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor);
			fprintf(fpdst, "[%d]( %f, %f)\n", knotidx,
				cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor,
				cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor);
		}

		fprintf(fpsrc, "\n");
		fprintf(fpdst, "\n");
	}
	fclose(fpsrc);
	fclose(fpdst);
}
#endif // SAVE_MESH_TBL_FILE

static CVI_S32 _convert_1st_src_midxy(int num_tilex_s1, CVI_U32 mesh_1st_size,
	COORD2D_INT_HW *src_1st_list, int tidx, int tidy, uint8_t *ptr,
	uint32_t *offset_p)
{
	uint32_t offset = *offset_p;
	CVI_U32 idx;

	for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
		for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
			int xidx = tidx * MESH_NUM_ATILE + midx;
			int yidx = tidy * MESH_NUM_ATILE + midy;

			if (offset >= (mesh_1st_size - 2))
				return CVI_ERR_GDC_ILLEGAL_PARAM;

			idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 0;
			ptr[offset++] = src_1st_list[idx].xcor[0];
			ptr[offset++] = src_1st_list[idx].xcor[1];
			ptr[offset++] = src_1st_list[idx].xcor[2];

			if (midx == MESH_NUM_ATILE - 1) {
				if (offset >= (mesh_1st_size - 2))
					return CVI_ERR_GDC_ILLEGAL_PARAM;

				idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 1;
				ptr[offset++] = src_1st_list[idx].xcor[0];
				ptr[offset++] = src_1st_list[idx].xcor[1];
				ptr[offset++] = src_1st_list[idx].xcor[2];
			}
		}

		if (midy == MESH_NUM_ATILE - 1) {
			for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
				int xidx = tidx * MESH_NUM_ATILE + midx;
				int yidx = tidy * MESH_NUM_ATILE + midy;

				if (offset >= (mesh_1st_size - 2))
					return CVI_ERR_GDC_ILLEGAL_PARAM;

				idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 3;
				ptr[offset++] = src_1st_list[idx].xcor[0];
				ptr[offset++] = src_1st_list[idx].xcor[1];
				ptr[offset++] = src_1st_list[idx].xcor[2];

				if (midx == MESH_NUM_ATILE - 1) {
					if (offset >= (mesh_1st_size - 2)) {
						return CVI_ERR_GDC_ILLEGAL_PARAM;
					}

					idx = (yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 + 2;
					ptr[offset++] = src_1st_list[idx].xcor[0];
					ptr[offset++] = src_1st_list[idx].xcor[1];
					ptr[offset++] = src_1st_list[idx].xcor[2];
				}
			}
		}
	}

	*offset_p = offset;

	return CVI_SUCCESS;
}

static CVI_S32 convert_1st_src_mesh_table(int num_tiley_s1, int num_tilex_s1,
	COORD2D_INT_HW *src_1st_list, COORD2D_INT_HW *src_1st_list_1d,
	CVI_U32 mesh_1st_size)
{
	// packed data in dram for HW
	uint8_t *ptr = (uint8_t *)src_1st_list_1d;
	uint32_t offset = 0;
	CVI_S32 ret;

	for (int tidy = 0; tidy < num_tiley_s1; tidy++) {
		for (int tidx = 0; tidx < num_tilex_s1; tidx++) {
			ret = _convert_1st_src_midxy(num_tilex_s1, mesh_1st_size,
						     src_1st_list, tidx, tidy, ptr,
						     &offset);
			if (ret != CVI_SUCCESS)
				return ret;

			// # 256 bytes per tile
			for (uint32_t i = 0; i < 13; i++)
				ptr[offset++] = 0xab;

		}
	}

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  offset=%d, mesh_1st_size=%d\n",
		       offset, mesh_1st_size);

#ifdef SAVE_MESH_TBL_FILE
	FILE *fp1x_bin = fopen("srcx_1st_mesh.bin", "wb");

	if (fp1x_bin) {
		size_t wr_size = fwrite(src_1st_list_1d, mesh_1st_size, 1, fp1x_bin);

		if (wr_size != mesh_1st_size)
			CVI_TRACE_GDC(CVI_DBG_DEBUG,
				"1st src mesh, fwrite %d, only %d succeed\n",
				mesh_1st_size, wr_size);
		fclose(fp1x_bin);
	}
#endif

	return CVI_SUCCESS;
}

static CVI_S32 _get_1st_src_midxy(int num_tilex_s1,
	LDC_ATTR *cfg, COORD2D_INT_HW *src_1st_list, int tidx, int tidy)
{
	CVI_U32 max_mesh_num = 9 * (MAX_HEIGHT_MESH_NUM * MAX_WIDTH_MESH_NUM);

	// go check all meshes inside.
	for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
		for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
			// each grid mesh from destination
			// find the mapping coordinate onto the source.
			COORD2D_INT cur_dst_mesh[4];

			memset(cur_dst_mesh, 0, sizeof(cur_dst_mesh));

			cur_dst_mesh[0].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;
			cur_dst_mesh[1].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
			cur_dst_mesh[2].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
			cur_dst_mesh[3].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;

			cur_dst_mesh[0].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
			cur_dst_mesh[1].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
			cur_dst_mesh[2].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;
			cur_dst_mesh[3].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;

			// for each knot of grid cur_dst_mesh, check all sw mesh
			// "on source" to estimate approximation.
			// swmesh_hit_index[N] is to record which sw-mesh index
			// cur knot-N hit.
			uint32_t swmesh_hit_index[4];

			for (int i = 0; i < 4; i++)
				swmesh_hit_index[i] = 0xFFFFFFFF;

			// go through al hwmesh knots
			for (int knotidx = 0; knotidx < 4; knotidx++) {
				for (uint32_t meshidx = 0; meshidx < max_mesh_num; meshidx++) {
					MESH_STRUCT cur_sw_mesh;
					MESH_STRUCT *dst = &cfg->DstRgnMeshInfoExt[meshidx];
					MESH_STRUCT *src = &cfg->SrcRgnMeshInfoExt[meshidx];

					// load cur_sw_mesh to check if hwmesh's knot is in it?
					for (int swknotidx = 0; swknotidx < 4; swknotidx++) {
						// 1st stage, SW destination( x, y ) = ( int ,float )
						// 1st stage, SW source( x,y ) = ( float, float )
						cur_sw_mesh.knot[swknotidx].xcor =
							dst->knot[swknotidx].xcor;
						cur_sw_mesh.knot[swknotidx].ycor =
							src->knot[swknotidx].ycor;
					}

					// check if cur-pixel in this mesh
					int sw_mesh_hit = _chk_in_mesh(cur_dst_mesh[knotidx].xcor,
							cur_dst_mesh[knotidx].ycor, cur_sw_mesh);
					if (sw_mesh_hit) {
						swmesh_hit_index[knotidx] = meshidx;
						break;
					}
				}

				// each knot has been
				// assigned a swmesh index
				// to estimate approximation.
				// do estimation here:
				COORD2D map_src_knot;

				memset(&map_src_knot, 0, sizeof(map_src_knot));

				if (swmesh_hit_index[knotidx] == 0xFFFFFFFF) {
					CVI_TRACE_GDC(CVI_DBG_ERR,
						       "    knotidx = %d tidx = %d tidy = %d,\n",
						       knotidx, tidx, tidy);
					CVI_TRACE_GDC(CVI_DBG_ERR, "    midx = %d, midy = %d,\n", midx,
						       midy);
					CVI_TRACE_GDC(CVI_DBG_ERR,
						       "    cur_dst_mesh[%d] = (%d, %d)\n", knotidx,
						       cur_dst_mesh[knotidx].xcor,
						       cur_dst_mesh[knotidx].ycor);
					CVI_TRACE_GDC(CVI_DBG_ERR, "    DEBUG STASRT!!\n");
					return CVI_ERR_GDC_ILLEGAL_PARAM;
				}

				map_src_knot = _find_knot_map2src(cur_dst_mesh[knotidx], cfg,
							swmesh_hit_index[knotidx], 0); //0 = 1st stage

				int xidx = tidx * MESH_NUM_ATILE + midx;
				int yidx = tidy * MESH_NUM_ATILE + midy;
				uint32_t xcor = (uint32_t)_double2Int_s13_10(map_src_knot.xcor);
				uint32_t offset =
					(yidx * (num_tilex_s1 * MESH_NUM_ATILE) + xidx) * 4 +
					knotidx;

				src_1st_list[offset].xcor[0] = xcor & 0xff;
				src_1st_list[offset].xcor[1] = (xcor >> 8) & 0xff;
				src_1st_list[offset].xcor[2] = (xcor >> 16) & 0xff;
			}
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 _offline_get_1st_src_mesh_table(int num_tiley_s1,
					       int num_tilex_s1,
					       LDC_ATTR *cfg,
					       COORD2D_INT_HW *src_1st_list)
{
	CVI_S32 ret;

	// 1st stage buffer data.
	for (int tidy = 0; tidy < num_tiley_s1; tidy++) {
		for (int tidx = 0; tidx < num_tilex_s1; tidx++) {
			ret = _get_1st_src_midxy(num_tilex_s1, cfg,
						 src_1st_list, tidx, tidy);
			if (ret != CVI_SUCCESS)
				return ret;
		}
		CVI_TRACE_GDC(CVI_DBG_DEBUG, "    OFFLINE tidy = %d\n", tidy);
	}

#ifdef SAVE_MESH_TBL_FILE
	save_1st_src_mesh_table(num_tiley_s1, num_tilex_s1, src_1st_list);
#endif

	return CVI_SUCCESS;
}

static void _convert_2nd_src_midxy(int num_tilex_s2,
	COORD2D_INT_HW *src_2nd_list, int tidx, int tidy, uint8_t *ptr,
	uint32_t *offset_ptr)
{
	uint32_t offset = *offset_ptr;
	CVI_U32 idx;

	for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
		for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
			int xidx = tidx * MESH_NUM_ATILE + midx;
			int yidx = tidy * MESH_NUM_ATILE + midy;

			idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 0;
			ptr[offset++] = src_2nd_list[idx].xcor[0];
			ptr[offset++] = src_2nd_list[idx].xcor[1];
			ptr[offset++] = src_2nd_list[idx].xcor[2];

			if (midx == MESH_NUM_ATILE - 1) {
				idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 1;
				ptr[offset++] = src_2nd_list[idx].xcor[0];
				ptr[offset++] = src_2nd_list[idx].xcor[1];
				ptr[offset++] = src_2nd_list[idx].xcor[2];
			}
		}

		if (midy == MESH_NUM_ATILE - 1) {
			for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
				int xidx = tidx * MESH_NUM_ATILE + midx;
				int yidx = tidy * MESH_NUM_ATILE + midy;

				idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 3;
				ptr[offset++] = src_2nd_list[idx].xcor[0];
				ptr[offset++] = src_2nd_list[idx].xcor[1];
				ptr[offset++] = src_2nd_list[idx].xcor[2];

				if (midx == MESH_NUM_ATILE - 1) {
					idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 + 2;
					ptr[offset++] = src_2nd_list[idx].xcor[0];
					ptr[offset++] = src_2nd_list[idx].xcor[1];
					ptr[offset++] = src_2nd_list[idx].xcor[2];
				}
			}
		}
	}

	*offset_ptr = offset;
}

static void convert_2nd_src_mesh_table(int num_tiley_s2, int num_tilex_s2,
				       COORD2D_INT_HW *src_2nd_list,
				       COORD2D_INT_HW *src_2nd_list_1d,
				       CVI_U32 mesh_2nd_size)
{
	// packed data in dram for HW
	uint8_t *ptr = (uint8_t *)src_2nd_list_1d;
	uint32_t offset = 0;
	uint32_t map_file_size = num_tiley_s2 * num_tilex_s2 * 256;

	if (mesh_2nd_size < map_file_size) {
		CVI_TRACE_GDC(CVI_DBG_ERR,
			       "error, mesh_2nd_size(%d) < map_file_size(%d)\n",
			       mesh_2nd_size, map_file_size);
		return;
	}

	for (int tidy = 0; tidy < num_tiley_s2; tidy++) {
		for (int tidx = 0; tidx < num_tilex_s2; tidx++) {
			_convert_2nd_src_midxy(num_tilex_s2, src_2nd_list, tidx,
					       tidy, ptr, &offset);

			// # 256 bytes per tile
			// fout.write(bytearray([0xab]*13))
			for (uint32_t i = 0; i < 13; i++)
				ptr[offset++] = 0xab;
		}
	}

#ifdef SAVE_MESH_TBL_FILE
	FILE *fp2x_bin = fopen("srcx_2nd_mesh.bin", "wb");
	size_t wr_size = fwrite(src_2nd_list_1d, mesh_2nd_size, 1, fp2x_bin);

	if (wr_size != mesh_2nd_size)
		CVI_TRACE_GDC(CVI_DBG_DEBUG,
			"2nd src mesh, fwrite %d, only %d succeed\n",
			mesh_2nd_size, wr_size);
	fclose(fp2x_bin);
#endif
}

static CVI_S32 _fill_src_2nd_list(int num_tilex_s2, LDC_ATTR *cfg,
	COORD2D_INT_HW *src_2nd_list, int tidx, int tidy, int midx, int midy,
	MESH_STRUCT *dstMesh, COORD2D_INT *cur_dst_mesh,
	uint32_t *swmesh_hit_index)
{
	CVI_U32 max_mesh_num = 9 * (MAX_HEIGHT_MESH_NUM * MAX_WIDTH_MESH_NUM);

	// go through all hwmesh knots
	for (int knotidx = 0; knotidx < 4; knotidx++) {
		for (CVI_U32 meshidx = 0; meshidx < max_mesh_num; meshidx++) {
			MESH_STRUCT cur_sw_mesh;

			// load cur_sw_mesh to check if hwmesh's knot is in it?
			for (int swknotidx = 0; swknotidx < 4; swknotidx++) {
				// 1st stage, SW destination( x, y ) = ( int ,float )
				// 1st stage, SW source( x,y ) = ( float, float )
				cur_sw_mesh.knot[swknotidx].xcor =
					dstMesh[meshidx].knot[swknotidx].xcor;
				cur_sw_mesh.knot[swknotidx].ycor =
					dstMesh[meshidx].knot[swknotidx].ycor;
			}

			// check if cur-pixel in this mesh
			int sw_mesh_hit = _chk_in_mesh(cur_dst_mesh[knotidx].xcor,
				cur_dst_mesh[knotidx].ycor, cur_sw_mesh);

			if (sw_mesh_hit) {
				swmesh_hit_index[knotidx] = meshidx;
				continue;
			}
		}

		// each knot has been assigned a swmesh index to estimate approximation.
		// do estimation here:
		// int knotMissingFlag = 0;
		COORD2D map_src_knot;

		memset(&map_src_knot, 0, sizeof(map_src_knot));
		//COORD2D_INT map_src_knot_32bit;
		if (swmesh_hit_index[knotidx] == 0xFFFFFFFF) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "    !!!! 2ndSTAG----ERROR !!!\n");
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}

		map_src_knot = _find_knot_map2src(cur_dst_mesh[knotidx], cfg,
				swmesh_hit_index[knotidx], 1); // 1: 2nd stage

		// update to buffer order should align to interpolation index:
		int xidx = tidx * MESH_NUM_ATILE + midx;
		int yidx = tidy * MESH_NUM_ATILE + midy;

		uint32_t xcor = (uint32_t)_double2Int_s13_10(map_src_knot.xcor);
		uint32_t idx = (yidx * (num_tilex_s2 * MESH_NUM_ATILE) + xidx) * 4 +
			       knotidx;

		src_2nd_list[idx].xcor[0] = xcor & 0xff;
		src_2nd_list[idx].xcor[1] = (xcor >> 8) & 0xff;
		src_2nd_list[idx].xcor[2] = (xcor >> 16) & 0xff;
	}

	return CVI_SUCCESS;
}

static CVI_S32 _offline_get_2nd_src_mesh_table(int stage2_rotate_type,
	int num_tiley_s2, int num_tilex_s2, LDC_ATTR *cfg,
	COORD2D_INT_HW *src_2nd_list, int src_width_s1, int src_height_s1,
	CVI_U32 mesh_2nd_size)
{
	// stage2_rotate_type = 0:  +90 degrees
	// stage2_rotate_type = 1:  -90 degrees

	CVI_U32 max_mesh_num = 9 * (MAX_HEIGHT_MESH_NUM * MAX_WIDTH_MESH_NUM);

	int RMATRIX[2][2];
	MESH_STRUCT *dstMesh = cfg->DstRgnMeshInfoExt2ND;
	MESH_STRUCT *srcMesh = cfg->SrcRgnMeshInfoExt2ND;

	CVI_S32 ret;

	if (stage2_rotate_type) {
		//-90 degrees
		RMATRIX[0][0] = 0;
		RMATRIX[0][1] = -1;
		RMATRIX[1][0] = 1;
		RMATRIX[1][1] = 0;
	} else {
		// + 90 degrees
		RMATRIX[0][0] = 0;
		RMATRIX[0][1] = 1;
		RMATRIX[1][0] = -1;
		RMATRIX[1][1] = 0;
	}

	for (CVI_U32 meshidx = 0; meshidx < max_mesh_num; meshidx++) {
		for (int knotidx = 0; knotidx < 4; knotidx++) {
			dstMesh[meshidx].knot[knotidx].xcor =
				RMATRIX[0][0] * (cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
				RMATRIX[0][1] * (cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

			dstMesh[meshidx].knot[knotidx].ycor =
				RMATRIX[1][0] *	(cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
				RMATRIX[1][1] * (cfg->DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

			srcMesh[meshidx].knot[knotidx].xcor =
				RMATRIX[0][0] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
				RMATRIX[0][1] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

			srcMesh[meshidx].knot[knotidx].ycor =
				RMATRIX[1][0] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor) +
				RMATRIX[1][1] * (cfg->SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor);

			if (RMATRIX[0][1] == 1) {
				dstMesh[meshidx].knot[knotidx].xcor += 0;
				dstMesh[meshidx].knot[knotidx].ycor += (src_width_s1 - 1);
				srcMesh[meshidx].knot[knotidx].xcor += 0;
				srcMesh[meshidx].knot[knotidx].ycor += (src_width_s1 - 1);
			} else {
				// +90
				dstMesh[meshidx].knot[knotidx].xcor += (src_height_s1 - 1);
				dstMesh[meshidx].knot[knotidx].ycor += 0;
				srcMesh[meshidx].knot[knotidx].xcor += (src_height_s1 - 1);
				srcMesh[meshidx].knot[knotidx].ycor += 0;
			}
		}
	}
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    rotate mesh table by 90 degree done!!\n");

	// 1st stage buffer data.
	for (int tidy = 0; tidy < num_tiley_s2; tidy++) {
		for (int tidx = 0; tidx < num_tilex_s2; tidx++) {
			// tmp buffer for dump data
			// for each tile
			// go check all meshes inside.
			for (int midy = 0; midy < MESH_NUM_ATILE; midy++) {
				for (int midx = 0; midx < MESH_NUM_ATILE; midx++) {
					// each grid mesh from destination
					// find the mapping coordinate onto the source.
					COORD2D_INT cur_dst_mesh[4];

					memset(cur_dst_mesh, 0, sizeof(cur_dst_mesh));

					cur_dst_mesh[0].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;
					cur_dst_mesh[1].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
					cur_dst_mesh[2].xcor = (tidx * TILESIZE) + (midx + 1) * HW_MESH_SIZE;
					cur_dst_mesh[3].xcor = (tidx * TILESIZE) + (midx + 0) * HW_MESH_SIZE;

					cur_dst_mesh[0].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
					cur_dst_mesh[1].ycor = (tidy * TILESIZE) + (midy + 0) * HW_MESH_SIZE;
					cur_dst_mesh[2].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;
					cur_dst_mesh[3].ycor = (tidy * TILESIZE) + (midy + 1) * HW_MESH_SIZE;

					// for each knot of grid cur_dst_mesh, check all sw mesh "on source" to
					// estimate approximation.
					// swmesh_hit_index[N] is to record which sw-mesh index cur knot-N hit.
					uint32_t swmesh_hit_index[4];

					for (int i = 0; i < 4; i++)
						swmesh_hit_index[i] = 0xFFFFFFFF;

					ret = _fill_src_2nd_list(num_tilex_s2, cfg, src_2nd_list,
								 tidx, tidy, midx, midy,
								 dstMesh, cur_dst_mesh, swmesh_hit_index);
					if (ret != CVI_SUCCESS)
						return ret;
				}
			}
		}
		CVI_TRACE_GDC(CVI_DBG_DEBUG, "    2nd Stage OFFLINE tidy = %d\n", tidy);
	}

#ifdef SAVE_MESH_TBL_FILE
	save_2nd_src_mesh_table(num_tiley_s2, num_tilex_s2, src_2nd_list, mesh_2nd_size);
#else
	(void)mesh_2nd_size;
#endif

	return CVI_SUCCESS;
}

static void _ldc_attr_map_cv182x(const LDC_ATTR_S *pstLDCAttr, LDC_ATTR *cfg,
				 LDC_RGN_ATTR *rgn_attr, double x0,
				 double y0, double r)
{
	// Global Initialization
	cfg->Enable = 1;
	cfg->OutW_disp = (int)x0 * 2; // -200;
	cfg->OutH_disp = (int)y0 * 2; // -200;
	cfg->InCenterX = (int)x0; // front-end set.
	cfg->InCenterY = (int)y0; // front-end set.
	cfg->InRadius = (int)r; // front-end set.

	cfg->SliceX_Num = 2;
	cfg->SliceY_Num = 2;

	cfg->RgnNum = 1;
	rgn_attr[0].RegionValid = 1;
	rgn_attr[0].ZoomV = pstLDCAttr->bAspect;
	rgn_attr[0].Pan = pstLDCAttr->s32XYRatio;
	rgn_attr[0].PanEnd = pstLDCAttr->s32DistortionRatio;
	rgn_attr[0].OutW = cfg->OutW_disp;
	rgn_attr[0].OutH = cfg->OutH_disp;
	rgn_attr[0].OutX = 0;
	rgn_attr[0].OutY = 0;
	rgn_attr[0].InRadius = pstLDCAttr->s32CenterXOffset;
	rgn_attr[0].OutRadius = pstLDCAttr->s32CenterYOffset;
	rgn_attr[0].MeshVer = 16;
	rgn_attr[0].MeshHor = 16;
	rgn_attr[0].ThetaX = pstLDCAttr->s32XRatio;
	rgn_attr[0].ThetaZ = pstLDCAttr->s32YRatio;
}

static CVI_S32 _get_region_dst_mesh_list(LDC_RGN_ATTR *rgn_attr, int view_w,
					 int view_h, int mesh_horcnt, int mesh_vercnt, int rgn_idx)
{
	if (rgn_attr[rgn_idx].RegionValid != 1)
		return CVI_ERR_GDC_ILLEGAL_PARAM;

	// 1st loop: to find mesh infos. on source ( backward projection )
	// hit index for buffer

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    view_h = %d,\n", view_h);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    view_w = %d,\n", view_w);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    mesh_horcnt = %d,\n", mesh_horcnt);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    mesh_vercnt = %d,\n", mesh_vercnt);

	int mesh_xstep = ((1 << PHASEBIT) * (view_w) / mesh_horcnt); //.2
	int mesh_ystep = ((1 << PHASEBIT) * (view_h) / mesh_vercnt); //.2

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    mesh_xstep = %d,\n", mesh_xstep);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    mesh_ystep = %d,\n", mesh_ystep);

	// give 3 types
	// 1. normal grid mesh geeration.
	// 2. center high resoution.
	// 3. boundary high resolution.
	// To extend to 3x size to cover miss hit area.

	for (int j = 0; j < mesh_vercnt; j++) {
		for (int i = 0; i < mesh_horcnt; i++) {
			int meshidx = j * mesh_horcnt + i;

			if (meshidx >= MAX_FRAME_MESH_NUM) {
				CVI_TRACE_GDC(CVI_DBG_WARN, "  [%d][%d] meshidx = %d >= MAX_FRAME_MESH_NUM(%d)\n",
						j, i, meshidx, MAX_FRAME_MESH_NUM);
				return CVI_ERR_GDC_ILLEGAL_PARAM;
			}

			int xcor = (mesh_xstep * (i + 0)) >> PHASEBIT;
			int xcor_r = (mesh_xstep * (i + 1)) >> PHASEBIT;
			int ycor = (mesh_ystep * (j + 0)) >> PHASEBIT;
			int ycor_d = (mesh_ystep * (j + 1)) >> PHASEBIT;

			xcor_r = MIN(view_w - 1, xcor_r);
			ycor_d = MIN(view_h - 1, ycor_d);

			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].xcor = xcor;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].ycor = ycor;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].xcor = xcor_r;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].ycor = ycor;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].xcor = xcor_r;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].ycor = ycor_d;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].xcor = xcor;
			rgn_attr[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].ycor = ycor_d;
		}

		if (j &&
			0 != (rgn_attr[rgn_idx].DstRgnMeshInfo[j * mesh_horcnt].knot[0].ycor -
			      rgn_attr[rgn_idx].DstRgnMeshInfo[(j - 1) * mesh_horcnt].knot[3].ycor)) {
			CVI_TRACE_GDC(CVI_DBG_WARN,
				       "    WARNING!!!!!!! Mesh are not tightly connected to each other!!!\n");
			CVI_TRACE_GDC(CVI_DBG_WARN, "    Check The Position: ");
			CVI_TRACE_GDC(CVI_DBG_WARN, "      [%d] DstRgnMeshInfo[%d * %d].knot[0].ycor = %10f\n",
					rgn_idx, j, mesh_horcnt,
					rgn_attr[rgn_idx].DstRgnMeshInfo[j * mesh_horcnt].knot[0].ycor);
			CVI_TRACE_GDC(CVI_DBG_WARN, "      [%d] DstRgnMeshInfo[(%d - 1) * %d].knot[3] = %10f\n",
					rgn_idx, j, mesh_horcnt,
					rgn_attr[rgn_idx].DstRgnMeshInfo[j * mesh_horcnt].knot[3].ycor);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	// Third part ----------------------------------------------------------
	// Extend Destination Mesh Grid to 3x3 large.
	// extend destination mesh here, off line work!!
	// Why need extend ?
	// 1st stage de-warp of pincushion case:
	//    our mapping is grid mesh from final destination to source.
	//    distorion of vertical area has no mesh covered on it.
	//    we need to create mesh mapping for these area, so to extend 3x3
	//    final destination meshes.
	// ---------------------------------------------------------------------
	for (int reorder_meshidx = 0; reorder_meshidx < 9 * (mesh_horcnt * mesh_vercnt); reorder_meshidx++) {
		int mesh_horcntExt = (mesh_horcnt * 3);

		int reorder_idy = (reorder_meshidx / mesh_horcntExt);
		int reorder_idx = reorder_meshidx - (reorder_idy * mesh_horcntExt);

		int ori_idx = reorder_idx % mesh_horcnt;
		int ori_idy = reorder_idy % mesh_vercnt;

		int ext_idx = (reorder_idx / mesh_horcnt) - 1; // -1, 0, +1
		int ext_idy = (reorder_idy / mesh_vercnt) - 1; // -1, 0, +1

		for (int knotidx = 0; knotidx < 4; knotidx++) {
			// view "-1": ex: ori: 1079, upside = 179 - 1079 = 0
			rgn_attr[0].DstRgnMeshInfoExt[reorder_meshidx].knot[knotidx].xcor =
				rgn_attr[0].DstRgnMeshInfo[ori_idy * mesh_horcnt +
				ori_idx].knot[knotidx].xcor + ext_idx * (view_w - 1);

			rgn_attr[0].DstRgnMeshInfoExt[reorder_meshidx].knot[knotidx].ycor =
				rgn_attr[0].DstRgnMeshInfo[ori_idy * mesh_horcnt +
				ori_idx].knot[knotidx].ycor + ext_idy * (view_h - 1);
		}
	}

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "    PASS2!!!!\n\r");
	return CVI_SUCCESS;
}

static COORD2D _find_knot_map2src(COORD2D_INT cur_dst_mesh, LDC_ATTR *cfg,
				  int swmesh_hit_index, int stageID)
{
	COORD2D knot_source;
	MESH_STRUCT cur_dst_sw_mesh;
	MESH_STRUCT cur_src_sw_mesh;
	MESH_STRUCT *dstMesh = cfg->DstRgnMeshInfoExt;
	MESH_STRUCT *srcMesh = cfg->SrcRgnMeshInfoExt;
	MESH_STRUCT *dstMesh2ND = cfg->DstRgnMeshInfoExt2ND;
	MESH_STRUCT *srcMesh2ND = cfg->SrcRgnMeshInfoExt2ND;

	//======================================================================
	// input current destination grid mesh
	// input cur_dst_mesh hit stage-destination mesh index
	// load cur stage-destination mesh by given mesh index
	// calculate vector coefficients a & b
	// By coefficients a & b, mapping onto the cooridnate on source frame
	//======================================================================

	// check which triangle cur-pxl locates in,
	for (int knotidx = 0; knotidx < 4; knotidx++) {
		if (stageID == 0) {
			cur_dst_sw_mesh.knot[knotidx].xcor = dstMesh[swmesh_hit_index].knot[knotidx].xcor;
			cur_dst_sw_mesh.knot[knotidx].ycor = srcMesh[swmesh_hit_index].knot[knotidx].ycor;

			cur_src_sw_mesh.knot[knotidx].xcor = srcMesh[swmesh_hit_index].knot[knotidx].xcor;
			cur_src_sw_mesh.knot[knotidx].ycor = srcMesh[swmesh_hit_index].knot[knotidx].ycor;
		} else {
			cur_dst_sw_mesh.knot[knotidx].xcor = dstMesh2ND[swmesh_hit_index].knot[knotidx].xcor;
			cur_dst_sw_mesh.knot[knotidx].ycor = dstMesh2ND[swmesh_hit_index].knot[knotidx].ycor;

			cur_src_sw_mesh.knot[knotidx].xcor = srcMesh2ND[swmesh_hit_index].knot[knotidx].xcor;
			cur_src_sw_mesh.knot[knotidx].ycor = dstMesh2ND[swmesh_hit_index].knot[knotidx].ycor;
		}
	}

	// fixed in side 01 & side 23
	double x01_a = (double)cur_dst_mesh.xcor - cur_dst_sw_mesh.knot[(0 + stageID) % 4].xcor;
	double x01_b = cur_dst_sw_mesh.knot[(1 + stageID) % 4].xcor - (double)cur_dst_mesh.xcor;

	COORD2D INTER_01, INTER_32;

	INTER_01.xcor = cur_dst_sw_mesh.knot[(0 + stageID) % 4].xcor +
			(cur_dst_sw_mesh.knot[(1 + stageID) % 4].xcor -
			cur_dst_sw_mesh.knot[(0 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);
	INTER_01.ycor = cur_dst_sw_mesh.knot[(0 + stageID) % 4].ycor +
			(cur_dst_sw_mesh.knot[(1 + stageID) % 4].ycor -
			cur_dst_sw_mesh.knot[(0 + stageID) % 4].ycor) * (x01_a) / (x01_a + x01_b);
	INTER_32.xcor = cur_dst_sw_mesh.knot[(3 + stageID) % 4].xcor +
			(cur_dst_sw_mesh.knot[(2 + stageID) % 4].xcor -
			cur_dst_sw_mesh.knot[(3 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);
	INTER_32.ycor = cur_dst_sw_mesh.knot[(3 + stageID) % 4].ycor +
			(cur_dst_sw_mesh.knot[(2 + stageID) % 4].ycor -
			cur_dst_sw_mesh.knot[(3 + stageID) % 4].ycor) * (x01_a) / (x01_a + x01_b);

	double y01_a = (double)cur_dst_mesh.ycor - INTER_01.ycor;
	double y01_b = INTER_32.ycor - (double)cur_dst_mesh.ycor;

	COORD2D INTER_01_SRC, INTER_32_SRC;

	INTER_01_SRC.xcor = cur_src_sw_mesh.knot[(0 + stageID) % 4].xcor +
			    (cur_src_sw_mesh.knot[(1 + stageID) % 4].xcor -
			    cur_src_sw_mesh.knot[(0 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);
	INTER_32_SRC.xcor = cur_src_sw_mesh.knot[(3 + stageID) % 4].xcor +
			    (cur_src_sw_mesh.knot[(2 + stageID) % 4].xcor -
			    cur_src_sw_mesh.knot[(3 + stageID) % 4].xcor) * (x01_a) / (x01_a + x01_b);

	knot_source.ycor = (double)cur_dst_mesh.ycor;
	knot_source.xcor = INTER_01_SRC.xcor + (INTER_32_SRC.xcor - INTER_01_SRC.xcor) * (y01_a) / (y01_a + y01_b);

	return knot_source;
}

static void _get_frame_mesh_list(LDC_ATTR *cfg, LDC_RGN_ATTR *rgn_attr)
{
	// pack all regions' mesh info, including src & dst.
	int rgnNum = cfg->RgnNum;
	int meshNumRgn[MAX_REGION_NUM];
	int frameMeshIdx = 0;

	for (int i = 0; i < rgnNum; i++) {
		if (rgn_attr[i].RegionValid == 1) {
			meshNumRgn[i] = (rgn_attr[i].MeshHor * rgn_attr[i].MeshVer);

			// go through each region loop
			for (int meshidx = 0; meshidx < 9 * meshNumRgn[i]; meshidx++) {
				// each mesh has 4 knots
				for (int knotidx = 0; knotidx < 4; knotidx++) {
					cfg->DstRgnMeshInfoExt[frameMeshIdx].knot[knotidx].xcor =
						rgn_attr[i].DstRgnMeshInfoExt[meshidx].knot[knotidx].xcor;
					cfg->DstRgnMeshInfoExt[frameMeshIdx].knot[knotidx].ycor =
						rgn_attr[i].DstRgnMeshInfoExt[meshidx].knot[knotidx].ycor;

					cfg->SrcRgnMeshInfoExt[frameMeshIdx].knot[knotidx].xcor =
						rgn_attr[i].SrcRgnMeshInfoExt[meshidx].knot[knotidx].xcor;
					cfg->SrcRgnMeshInfoExt[frameMeshIdx].knot[knotidx].ycor =
						rgn_attr[i].SrcRgnMeshInfoExt[meshidx].knot[knotidx].ycor;
				}
				frameMeshIdx += 1;
			}
		}
	}

	// update mesh index number
	cfg->TotalMeshNum = frameMeshIdx;

#ifdef SAVE_MESH_TBL_FILE
	save_mesh_info(cfg);
#endif
}

static void _get_region_src_mesh_list(LDC_RGN_ATTR *rgn_attr, int rgn_idx,
				      double x0, double y0)
{
	int view_w = rgn_attr[rgn_idx].OutW;
	int view_h = rgn_attr[rgn_idx].OutH;
	int mesh_horcnt = rgn_attr[rgn_idx].MeshHor;
	int mesh_vercnt = rgn_attr[rgn_idx].MeshVer;

	// register:
	bool bAspect		= (bool)rgn_attr[0].ZoomV;
	int XYRatio		= minmax(rgn_attr[0].Pan, 0, 100);
	int XRatio		= minmax(rgn_attr[0].ThetaX, 0, 100);
	int YRatio		= minmax(rgn_attr[0].ThetaZ, 0, 100);
	int CenterXOffset	= minmax(rgn_attr[0].InRadius, -511, 511);
	int CenterYOffset	= minmax(rgn_attr[0].OutRadius, -511, 511);
	int DistortionRatio = minmax(rgn_attr[0].PanEnd, -300, 500);

	double norm = sqrt((view_w / 2) * (view_w / 2) + (view_h / 2) * (view_h / 2));

	// internal link to register:
	double k = (double)DistortionRatio / 1000.0;
	double ud_gain = (1 + k * (((double)view_h / 2) * (double)view_h / 2) / norm / norm);
	double lr_gain = (1 + k * (((double)view_w / 2) * (double)view_w / 2) / norm / norm);

	double Aspect_gainX = MAX(ud_gain, lr_gain);
	double Aspect_gainY = MAX(ud_gain, lr_gain);
	Vector2D dist2d;

	for (int i = 0; i < 9*(mesh_horcnt * mesh_vercnt); i++) {
		// Do LDC mapping in Center Mesh Grid only
		for (int knotidx = 0; knotidx < 4; knotidx++) {
			double x = rgn_attr[rgn_idx].DstRgnMeshInfoExt[i].knot[knotidx].xcor -
				   rgn_attr[rgn_idx].OutX - rgn_attr[rgn_idx].OutW / 2;
			double y = rgn_attr[rgn_idx].DstRgnMeshInfoExt[i].knot[knotidx].ycor -
				   rgn_attr[rgn_idx].OutY - rgn_attr[rgn_idx].OutH / 2;

			x = x - CenterXOffset; // -view_w / 2 + 1, view_w / 2 - 1);
			y = y - CenterYOffset; // -view_h / 2 + 1, view_h / 2 - 1);

			x = x / Aspect_gainX;
			y = y / Aspect_gainY;

			if (bAspect == true) {
				x = x * (1 - 0.333 * (100 - XYRatio) / 100);
				y = y * (1 - 0.333 * (100 - XYRatio) / 100);
			} else {
				x = x * (1 - 0.333 * (100 - XRatio) / 100);
				y = y * (1 - 0.333 * (100 - YRatio) / 100);
			}

			double rd = MIN(norm, sqrt(x * x + y * y));

			dist2d.x = x * (1 + k * ((rd / norm) * (rd / norm)));
			dist2d.y = y * (1 + k * ((rd / norm) * (rd / norm)));

			// update source mesh-info here
			rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[knotidx].xcor = dist2d.x + x0 + CenterXOffset;
			rgn_attr[rgn_idx].SrcRgnMeshInfoExt[i].knot[knotidx].ycor = dist2d.y + y0 + CenterYOffset;
		}
	}
}

static int _chk_location_to_line(COORD2D *meshcorA, COORD2D *meshcorB, int x,
				 int y, int inc_zero)
{
	int onright = 0;

	// along vector from A -> B
	// A(x1, y1) to B(x2, y2), P(x0�Ay0) outsize the line
	// determine which side the point resides
	// a = (x2 - x1, y2 - y1)
	// b = (x0 - x1, y0 - y1)

	double x1 = meshcorA->xcor;
	double y1 = meshcorA->ycor;
	double x2 = meshcorB->xcor;
	double y2 = meshcorB->ycor;

	double tmp = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);

	// has optional inclusion of zero as in mesh.
	onright = inc_zero ? (tmp >= 0) : (tmp > 0);

	return onright;
}

static int _double2Int_s13_10(double value)
{
#ifdef TWOS_COMPLEMENT
	//double abs_value = abs(value);
	//double dtmp2cpl = abs_value * 1024;
	int tmp2cpl = (int)(abs((int)(value * (1 << QFORMAT_M))));

	// get 24 bit
	tmp2cpl = tmp2cpl & 0xFFFFFF;

	int rtn_value = tmp2cpl;

	if (value < 0)
		rtn_value = (~tmp2cpl) + 1;
	else
		rtn_value = rtn_value;

	rtn_value = rtn_value & 0xFFFFFF;

#else
	int sbit = (value < 0) ? 1 : 0;
	int tmp2cpl = (int)(abs(value) * (1 << QFORMAT_M));
	int rtn_value = sbit << (13 + QFORMAT_M) | tmp2cpl; // s13.QFORMAT_M
#endif
	return rtn_value;
}

static int _chk_in_mesh(int x, int y, MESH_STRUCT cur_sw_mesh)
{
	int inthismesh_hit = 0;
	int onright_01 = _chk_location_to_line(&cur_sw_mesh.knot[0], &cur_sw_mesh.knot[1], x, y, 1);
	int onright_12 = _chk_location_to_line(&cur_sw_mesh.knot[1], &cur_sw_mesh.knot[2], x, y, 0);
	int onright_23 = _chk_location_to_line(&cur_sw_mesh.knot[2], &cur_sw_mesh.knot[3], x, y, 0);
	int onright_30 = _chk_location_to_line(&cur_sw_mesh.knot[3], &cur_sw_mesh.knot[0], x, y, 1);

	// in mesh case: clockwise: 0 -> 1 -> 2 -> 3
	//     & counter clockwise: 3 -> 2 -> 1 -> 0
	// onright_01, onright_12, onright_23, onright_30, = ( 1,  1,  1,  1  )
	// onright_01, onright_12, onright_23, onright_30, = ( 0, 0, 0, 0 )

	// inside the mesh
	if ((onright_01 == onright_12) && (onright_12 == onright_23) && (onright_23 == onright_30)) {
		inthismesh_hit = 1;
	} else {
		inthismesh_hit = 0;
	}
	return inthismesh_hit;
}

void mesh_gen_rotation(SIZE_S in_size, SIZE_S out_size, ROTATION_E rot,
		       uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
	(void)in_size;
	(void)out_size;
	(void)rot;
	(void)mesh_phy_addr;
	(void)mesh_vir_addr;

	// No need to generate mesh
}

void mesh_gen_fisheye(SIZE_S in_size, SIZE_S out_size,
		      const FISHEYE_ATTR_S *pstFisheyeAttr,
		      uint64_t mesh_phy_addr, void *mesh_vir_addr,
		      ROTATION_E rot)
{
	(void)in_size;
	(void)out_size;
	(void)pstFisheyeAttr;
	(void)mesh_phy_addr;
	(void)mesh_vir_addr;
	(void)rot;
}

CVI_S32 mesh_gen_ldc(SIZE_S in_size, SIZE_S out_size,
	const LDC_ATTR_S *pstLDCAttr, uint64_t mesh_phy_addr,
	void *mesh_vir_addr, ROTATION_E rot)
{
	CVI_S32 ret = CVI_SUCCESS;
	_reg_dwa reg;
	COORD2D_INT_HW *src_1st_list_1d = NULL, *src_2nd_list_1d = NULL;
	COORD2D_INT_HW *src_1st_list = NULL, *src_2nd_list = NULL;
	LDC_ATTR *cfg = NULL;
	LDC_RGN_ATTR *rgn_attr = NULL;
#ifdef ENABLE_PROFILE
	struct timespec start, end;
#endif

	(void)mesh_phy_addr;
	(void)rot;

	CVI_U32 mesh_1st_size = 0, mesh_2nd_size = 0;

	mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
	src_1st_list_1d = (COORD2D_INT_HW *)mesh_vir_addr;
	src_2nd_list_1d = (COORD2D_INT_HW *)(mesh_vir_addr + mesh_1st_size);

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  1st src_1st_list=%p(%d), src_2nd_list=%p(%d)\n",
		       src_1st_list_1d, mesh_1st_size, src_2nd_list_1d, mesh_2nd_size);

	src_1st_list = (COORD2D_INT_HW *)malloc(mesh_1st_size);
	src_2nd_list = (COORD2D_INT_HW *)malloc(mesh_2nd_size);
	cfg = (LDC_ATTR *)calloc(1, sizeof(*cfg));
	rgn_attr = (LDC_RGN_ATTR *)calloc(1, sizeof(*rgn_attr) * MAX_REGION_NUM);
	if (!src_1st_list || !src_2nd_list || !cfg || !rgn_attr) {
		free(src_1st_list);
		free(src_2nd_list);
		free(cfg);
		free(rgn_attr);

		CVI_TRACE_GDC(CVI_DBG_ERR, "  fail to alloc mesh\n");
		return CVI_ERR_GDC_NOMEM;
	}

	// al registers
	reg.dwa_en = 1;
	reg.stage2_rotate_type = 0;

	int bgc_pack = 0x00217E;

	reg.bg_color_y_r = (bgc_pack & 0xFF0000) >> 16;
	reg.bg_color_u_g = (bgc_pack & 0x00FF00) >> 8;
	reg.bg_color_v_b = (bgc_pack & 0x0000FF) >> 0;

	int ori_src_width = in_size.u32Width;
	int ori_src_height = in_size.u32Height;

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  ori_src_width = %d,\n", ori_src_width);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  ori_src_height = %d,\n", ori_src_height);

	// In LDC Processing, width & height  aligned to TILESIZE **
	int src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
	int src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  src_width_s1 = %d,\n", src_width_s1);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  src_height_s1 = %d,\n", src_height_s1);

	// auto assign correct range:
	reg.src_xstr_s1 = 0;
	reg.src_xend_s1 = ori_src_width - 1;
	reg.src_xstr_s2 = reg.stage2_rotate_type ? (src_height_s1 - ori_src_height) : 0;
	reg.src_xend_s2 = reg.stage2_rotate_type ? (src_height_s1 - 1) : (ori_src_height - 1);

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.stage2_rotate_type = %d,\n", reg.stage2_rotate_type);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.bg_color_y_r  = %d,\n", reg.bg_color_y_r);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.bg_color_u_g  = %d,\n", reg.bg_color_u_g);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.bg_color_v_b  = %d,\n", reg.bg_color_v_b);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.src_xstart_s1 = %d,\n", reg.src_xstr_s1);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.src_xend_s1   = %d,\n", reg.src_xend_s1);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.src_xstart_s2 = %d,\n", reg.src_xstr_s2);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  reg.src_xend_s2   = %d,\n", reg.src_xend_s2);

	int x0, y0, r;

	x0 = ori_src_width / 2;
	y0 = ori_src_height / 2;
	r = MIN(x0, y0);

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "cfg size %d\n", (int)sizeof(LDC_ATTR));

	// update parameters
	_ldc_attr_map_cv182x(pstLDCAttr, cfg, rgn_attr, x0, y0, r);

	// In Each Mode, for Every Region:
	for (int rgn_idx = 0; rgn_idx < cfg->RgnNum; rgn_idx++) {
		// check region valid first
		if (!rgn_attr[rgn_idx].RegionValid)
			return CVI_ERR_GDC_ILLEGAL_PARAM;

		// Destination Mesh-Info Allocation
		int view_w = rgn_attr[rgn_idx].OutW;
		int view_h = rgn_attr[rgn_idx].OutH;
		int mesh_horcnt = rgn_attr[rgn_idx].MeshHor;
		int mesh_vercnt = rgn_attr[rgn_idx].MeshVer;

		// get & store region mesh info.
#ifdef ENABLE_PROFILE
		clock_gettime(CLOCK_MONOTONIC, &start);
#endif
		ret |= _get_region_dst_mesh_list(rgn_attr, view_w, view_h, mesh_horcnt, mesh_vercnt, rgn_idx);
#ifdef ENABLE_PROFILE
		clock_gettime(CLOCK_MONOTONIC, &end);
		CVI_TRACE_GDC(CVI_DBG_ERR, "    _get_region_dst_mesh_list: %ldms\n",
				get_diff_in_us(start, end) / 1000);
#endif

		// Get Source Mesh-Info Projected from Destination by Differet ViewModw.
#ifdef ENABLE_PROFILE
		clock_gettime(CLOCK_MONOTONIC, &start);
#endif
		_get_region_src_mesh_list(rgn_attr, rgn_idx, x0, y0); //, mat0);
#ifdef ENABLE_PROFILE
		clock_gettime(CLOCK_MONOTONIC, &end);
		CVI_TRACE_GDC(CVI_DBG_ERR, "    _get_region_src_mesh_list: %ldms\n",
				get_diff_in_us(start, end) / 1000);
#endif

		// debug msg
		CVI_TRACE_GDC(CVI_DBG_DEBUG, "  REGION %d done.\n", rgn_idx);
	}

	//combine all region meshs - mesh projection done.
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif
	_get_frame_mesh_list(cfg, rgn_attr);
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &end);
	CVI_TRACE_GDC(CVI_DBG_ERR, "    _get_frame_mesh_list: %ldms\n",
			get_diff_in_us(start, end) / 1000);
#endif

	// modify frame size
	int dst_height_s1 = src_height_s1;
	int dst_width_s1 = src_width_s1;
	int src_height_s2 = dst_width_s1;
	int src_width_s2 = dst_height_s1;
	int dst_height_s2 = src_height_s2;
	int dst_width_s2 = src_width_s2;

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  dst_height_s1 = %d,\n", dst_height_s1);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  dst_width_s1  = %d,\n", dst_width_s1);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  src_height_s2 = %d,\n", src_height_s2);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  src_width_s2  = %d,\n", src_width_s2);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  dst_height_s2 = %d,\n", dst_height_s2);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  dst_width_s2  = %d,\n", dst_width_s2);

	// 1st-stage, in(1984, 1088 ) -> out(1984, 1088)
	// 2nd-stage, in(1088, 1984 ) -> out(1088, 1984)

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  1st-stage, in(%d, %d ) -> out(%d, %d)\n",
		       src_width_s1, src_height_s1, dst_width_s1, dst_height_s1);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  2nd-stage, in(%d, %d ) -> out(%d, %d)\n",
		       src_width_s2, src_height_s2, dst_width_s2, dst_height_s2);

	// tileNum: 1st stage =( 31 X 17 )   2nd stage = ( 17 X 31 )

	int num_tilex_s1 = dst_width_s1 / TILESIZE;
	int num_tiley_s1 = dst_height_s1 / TILESIZE;
	int num_tilex_s2 = dst_width_s2 / TILESIZE;
	int num_tiley_s2 = dst_height_s2 / TILESIZE;

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "  tileNum: 1st stage =( %d X %d )   2nd stage = ( %d X %d )\n",
		       num_tilex_s1, num_tiley_s1, num_tilex_s2, num_tiley_s2);

	// to calculate each stage source mesh x-data.
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif
	ret |= _offline_get_1st_src_mesh_table(num_tiley_s1, num_tilex_s1, cfg, src_1st_list);
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &end);
	CVI_TRACE_GDC(CVI_DBG_ERR, "    _offline_get_1st_src_mesh_table: %ldms\n",
			get_diff_in_us(start, end) / 1000);
#endif

#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif
	ret |= _offline_get_2nd_src_mesh_table(reg.stage2_rotate_type, num_tiley_s2, num_tilex_s2, cfg,
					       src_2nd_list, src_width_s1, src_height_s1, mesh_2nd_size);
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &end);
	CVI_TRACE_GDC(CVI_DBG_ERR, "    _offline_get_2nd_src_mesh_table: %ldms\n",
			get_diff_in_us(start, end) / 1000);
#endif

#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif
	convert_1st_src_mesh_table(num_tiley_s1, num_tilex_s1, src_1st_list, src_1st_list_1d, mesh_1st_size);
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &end);
	CVI_TRACE_GDC(CVI_DBG_ERR, "    convert_1st_src_mesh_table: %ldms\n",
			get_diff_in_us(start, end) / 1000);
#endif

#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif
	convert_2nd_src_mesh_table(num_tiley_s2, num_tilex_s2, src_2nd_list, src_2nd_list_1d, mesh_2nd_size);
#ifdef ENABLE_PROFILE
	clock_gettime(CLOCK_MONOTONIC, &end);
	CVI_TRACE_GDC(CVI_DBG_ERR, "    convert_2nd_src_mesh_table: %ldms\n",
			get_diff_in_us(start, end) / 1000);
#endif

	free(src_1st_list);
	free(src_2nd_list);
	free(cfg);
	free(rgn_attr);

	return ret;
}

void mesh_gen_cnv(const float *pfmesh_data, SIZE_S in_size, SIZE_S out_size,
		  const FISHEYE_ATTR_S *pstFisheyeAttr, uint64_t mesh_phy_addr,
		  void *mesh_vir_addr)
{
	(void)pfmesh_data;
	(void)in_size;
	(void)out_size;
	(void)pstFisheyeAttr;
	(void)mesh_phy_addr;
	(void)mesh_vir_addr;
}
