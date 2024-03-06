#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include <pthread.h>
#include <inttypes.h>

#include "cvi_base.h"
#include <linux/cvi_common.h>
#include <linux/cvi_math.h>
#include <linux/cvi_comm_video.h>

#include "cvi_gdc.h"
#include "gdc_mesh.h"


#define DEWARP_COORD_MBITS 13
#define DEWARP_COORD_NBITS 18
#define NUMBER_Y_LINE_A_SUBTILE 4

#define MESH_ID_FST 0xfffa // frame start
#define MESH_ID_FSS 0xfffb // slice start
#define MESH_ID_FSP 0xfffc // tile start
#define MESH_ID_FTE 0xfffd // tile end
#define MESH_ID_FSE 0xfffe // slice end
#define MESH_ID_FED 0xffff // frame end
#define MESH_HOR_DEFAULT 16
#define MESH_VER_DEFAULT 16
#define MESH_MAX_SIZE (32 * 32)
#define CLIP(x, min, max) MAX(MIN(x, max), min)

static int meshHor = MESH_HOR_DEFAULT;
static int meshVer = MESH_HOR_DEFAULT;
//  0--1
//  |  |
//  2--3
int MESH_EDGE[4][2] = {
	{ 0, 1 },
	{ 1, 3 },
	{ 2, 0 },
	{ 3, 2 }
};

int mesh_coordinate_float2fixed(float src_x_mesh_tbl[][4], float src_y_mesh_tbl[][4], float dst_x_mesh_tbl[][4],
				float dst_y_mesh_tbl[][4], int mesh_tbl_num, int isrc_x_mesh_tbl[][4],
				int isrc_y_mesh_tbl[][4], int idst_x_mesh_tbl[][4], int idst_y_mesh_tbl[][4])
{
	int64_t MAX_VAL;
	int64_t MIN_VAL;
	double tmp_val;
	int64_t val;

	MAX_VAL = 1;
	MAX_VAL <<= (DEWARP_COORD_MBITS + DEWARP_COORD_NBITS);
	MAX_VAL -= 1;
	MIN_VAL = -1 * MAX_VAL;

	for (int i = 0; i < mesh_tbl_num; i++) {
		for (int j = 0; j < 4; j++) {
			tmp_val = (src_x_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			isrc_x_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

			tmp_val = (src_y_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			isrc_y_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

			tmp_val = (dst_x_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			idst_x_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

			tmp_val = (dst_y_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			idst_y_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);
		}
	}

	return 0;
}

int mesh_scan_preproc_3(int dst_width, int dst_height
	, const float dst_x_mesh_tbl[][4], const float dst_y_mesh_tbl[][4]
	, int mesh_tbl_num, uint16_t *mesh_scan_id_order, int X_TILE_NUMBER
	, int NUM_Y_LINE_A_SUBTILE, int Y_TILE_NUMBER, int MAX_MESH_NUM_A_TILE)
{
	int tile_idx_x, tile_idx_y;
	int current_dst_mesh_y_line_intersection_cnt, mesh_num_cnt = 0;

	int id_idx = 0;
	int tmp_mesh_cnt = MAX_MESH_NUM_A_TILE * 4;
	int *tmp_mesh_scan_id_order = (int *)malloc(sizeof(int) * (MAX_MESH_NUM_A_TILE * 4));
	int NUM_X_LINE_A_TILE = ceil((float)dst_width / (float)X_TILE_NUMBER / 2.0) * 2;
	int NUM_Y_LINE_A_TILE =
		(int)(ceil(ceil((float)dst_height / (float)Y_TILE_NUMBER) / (float)NUMBER_Y_LINE_A_SUBTILE)) *
		NUMBER_Y_LINE_A_SUBTILE;

	mesh_scan_id_order[id_idx++] = MESH_ID_FST;

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "X_TILE_NUMBER(%d) Y_TILE_NUMBER(%d)\n", X_TILE_NUMBER, Y_TILE_NUMBER);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "NUM_X_LINE_A_TILE(%d) NUM_Y_LINE_A_TILE(%d)\n"
		, NUM_X_LINE_A_TILE, NUM_Y_LINE_A_TILE);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "NUM_Y_LINE_A_SUBTILE(%d)\n", NUM_Y_LINE_A_SUBTILE);

	for (tile_idx_y = 0; tile_idx_y < Y_TILE_NUMBER; tile_idx_y++) {
		// src_y = (1st line of this tile )
		// dst_y = (1st line of next tile )
		int src_y = CLIP((tile_idx_y * NUM_Y_LINE_A_TILE), 0, dst_height);
		int dst_y = CLIP((src_y + NUM_Y_LINE_A_TILE), 0, dst_height);

		for (tile_idx_x = 0; tile_idx_x < X_TILE_NUMBER; tile_idx_x++) {
			mesh_scan_id_order[id_idx++] = MESH_ID_FSS;

			int src_x = CLIP((tile_idx_x * NUM_X_LINE_A_TILE), 0, dst_width);
			int dst_x = CLIP((src_x + NUM_X_LINE_A_TILE), 0, dst_width);

			// label starting position (slice) and its size ( x & y )
			mesh_scan_id_order[id_idx++] = src_x;
			mesh_scan_id_order[id_idx++] = dst_x - src_x;
			mesh_scan_id_order[id_idx++] = src_y;
			mesh_scan_id_order[id_idx++] = dst_y - src_y;

			for (int y = src_y; y < dst_y; y++) {
				// if first line of a tile, then initialization
				if (y % NUM_Y_LINE_A_SUBTILE == 0) {
					mesh_num_cnt = 0;
					for (int i = 0; i < tmp_mesh_cnt; i++)
						tmp_mesh_scan_id_order[i] = -1;

					// frame separator ID insertion
					mesh_scan_id_order[id_idx++] = MESH_ID_FSP;
				}

				for (int m = 0; m < mesh_tbl_num; m++) {
					current_dst_mesh_y_line_intersection_cnt = 0;
					int is_mesh_incorp_yline = 0;
					int min_x = MIN(MIN(dst_x_mesh_tbl[m][0], dst_x_mesh_tbl[m][1]),
							MIN(dst_x_mesh_tbl[m][2], dst_x_mesh_tbl[m][3]));
					int max_x = MAX(MAX(dst_x_mesh_tbl[m][0], dst_x_mesh_tbl[m][1]),
							MAX(dst_x_mesh_tbl[m][2], dst_x_mesh_tbl[m][3]));
					int min_y = MIN(MIN(dst_y_mesh_tbl[m][0], dst_y_mesh_tbl[m][1]),
							MIN(dst_y_mesh_tbl[m][2], dst_y_mesh_tbl[m][3]));
					int max_y = MAX(MAX(dst_y_mesh_tbl[m][0], dst_y_mesh_tbl[m][1]),
							MAX(dst_y_mesh_tbl[m][2], dst_y_mesh_tbl[m][3]));
					if (min_y <= y && y <= max_y && min_x <= src_x && dst_x <= max_x)
						is_mesh_incorp_yline = 1;

					if (!is_mesh_incorp_yline) {
						for (int k = 0; k < 4; k++) {
							float knot_dst_a_y = dst_y_mesh_tbl[m][MESH_EDGE[k][0]];
							float knot_dst_b_y = dst_y_mesh_tbl[m][MESH_EDGE[k][1]];
							float knot_dst_a_x = dst_x_mesh_tbl[m][MESH_EDGE[k][0]];
							float knot_dst_b_x = dst_x_mesh_tbl[m][MESH_EDGE[k][1]];
							float delta_a_y = (float)y - knot_dst_a_y;
							float delta_b_y = (float)y - knot_dst_b_y;
							int intersect_x = 0;

							if ((src_x <= knot_dst_a_x) && (knot_dst_a_x <= dst_x))
								intersect_x = 1;
							if ((src_x <= knot_dst_b_x) && (knot_dst_b_x <= dst_x))
								intersect_x = 1;

							// check whether if vertex connection
							if ((delta_a_y == 0.f) && (intersect_x == 1)) {
								current_dst_mesh_y_line_intersection_cnt += 2;
							}
							// check whether if edge connection
							else if ((delta_a_y * delta_b_y < 0) && (intersect_x == 1)) {
								current_dst_mesh_y_line_intersection_cnt += 2;
							}
							// otherwise no connection
						} // finish check in a mesh
					}

					if ((current_dst_mesh_y_line_intersection_cnt > 0) || (is_mesh_incorp_yline == 1)) {
						// check the mesh in list or not
						int isInList = 0;

						for (int i = 0; i < mesh_num_cnt; i++) {
							if (m == tmp_mesh_scan_id_order[i]) {
								isInList = 1;
								break;
							}
						}
						// not in the list, then add the mesh to list
						if (!isInList) {
							tmp_mesh_scan_id_order[mesh_num_cnt] = m;
							mesh_num_cnt++;
						}
					}
				}

				// x direction reorder
				if (((y % NUM_Y_LINE_A_SUBTILE) == (NUM_Y_LINE_A_SUBTILE - 1)) || (y == dst_height - 1)) {
					for (int i = 0; i < mesh_num_cnt - 1; i++) {
						for (int j = 0; j < mesh_num_cnt - 1 - i; j++) {
							int m0 = tmp_mesh_scan_id_order[j];
							int m1 = tmp_mesh_scan_id_order[j + 1];
							float knot_m0_x0 = dst_x_mesh_tbl[m0][0];
							float knot_m0_x1 = dst_x_mesh_tbl[m0][1];
							float knot_m0_x2 = dst_x_mesh_tbl[m0][2];
							float knot_m0_x3 = dst_x_mesh_tbl[m0][3];
							float knot_m1_x0 = dst_x_mesh_tbl[m1][0];
							float knot_m1_x1 = dst_x_mesh_tbl[m1][1];
							float knot_m1_x2 = dst_x_mesh_tbl[m1][2];
							float knot_m1_x3 = dst_x_mesh_tbl[m1][3];

							int m0_min_x = MIN(MIN(knot_m0_x0, knot_m0_x1),
									   MIN(knot_m0_x2, knot_m0_x3));
							int m1_min_x = MIN(MIN(knot_m1_x0, knot_m1_x1),
									   MIN(knot_m1_x2, knot_m1_x3));

							if (m0_min_x > m1_min_x) {
								int tmp = tmp_mesh_scan_id_order[j];

								tmp_mesh_scan_id_order[j] =
									tmp_mesh_scan_id_order[j + 1];
								tmp_mesh_scan_id_order[j + 1] = tmp;
							}
						}
					}

					// mesh ID insertion
					for (int i = 0; i < mesh_num_cnt; i++) {
						mesh_scan_id_order[id_idx++] = tmp_mesh_scan_id_order[i];
					}

					// tile end ID insertion
					mesh_scan_id_order[id_idx++] = MESH_ID_FTE;
				}
			}

			// frame slice end ID insertion
			mesh_scan_id_order[id_idx++] = MESH_ID_FSE;
		}
	}

	// frame end ID insertion
	mesh_scan_id_order[id_idx++] = MESH_ID_FED;

	free(tmp_mesh_scan_id_order);

	return id_idx;
}

int mesh_tbl_reorder_and_parse_3(uint16_t *mesh_scan_tile_mesh_id_list, int mesh_id_list_entry_num,
				 int isrc_x_mesh_tbl[][4], int isrc_y_mesh_tbl[][4], int idst_x_mesh_tbl[][4],
				 int idst_y_mesh_tbl[][4], int X_TILE_NUMBER,
				 int Y_TILE_NUMBER, int Y_SUBTILE_NUMBER, int **reorder_mesh_tbl,
				 int *reorder_mesh_tbl_entry_num, uint16_t *reorder_mesh_id_list,
				 int *reorder_mesh_id_list_entry_num, uint64_t mesh_tbl_phy_addr)
{
	int mesh = -1;
	int reorder_mesh;
	int mesh_id_idx = 0;
	int mesh_idx = 0;
	int *reorder_id_map = (int *)malloc(sizeof(int) * 128 * 128);

	int *reorder_mesh_slice_tbl = reorder_mesh_tbl[0]; // initial mesh_tbl to

	int i = 0;
	//int ext_mem_addr_alignment = 32;

	(void) mesh_id_list_entry_num;
	(void)Y_SUBTILE_NUMBER;

	// frame start ID
	reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];

	for (int k = 0; k < Y_TILE_NUMBER; k++) {
		for (int j = 0; j < X_TILE_NUMBER; j++) {
			reorder_mesh_tbl[j + k * X_TILE_NUMBER] = reorder_mesh_slice_tbl + mesh_idx;

			reorder_mesh = 0;
#if 1 // (JAMMY) clear to -1
			for (int l = 0; l < 128 * 128; l++) {
				reorder_id_map[l] = -1;
			}
#else
			memset(reorder_id_map, 0xff, sizeof(sizeof(int) * 128 * 128));
#endif

			// slice start ID
			mesh = mesh_scan_tile_mesh_id_list[i++];
			reorder_mesh_id_list[mesh_id_idx++] = mesh;

#if 0 // (JAMMY) replace with phy-addr later.
			// reorder mesh table address -> reorder mesh id list header
			uintptr_t addr = (uintptr_t)reorder_mesh_tbl[j + k * X_TILE_NUMBER];

			reorder_mesh_id_list[mesh_id_idx++] = addr & 0x0000000fff;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0000fff000) >> 12;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0fff000000) >> 24;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0xf000000000) >> 36;
#else
			CVI_U64 addr = mesh_tbl_phy_addr
				+ ((uintptr_t)reorder_mesh_tbl[j + k * X_TILE_NUMBER]
				  - (uintptr_t)reorder_mesh_tbl[0]);

			reorder_mesh_id_list[mesh_id_idx++] = addr & 0x0000000fff;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0000fff000) >> 12;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0fff000000) >> 24;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0xf000000000) >> 36;
#endif

			// slice src and width -> reorder mesh id list header
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile x src
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile width
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile y src
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile height

			while (mesh_scan_tile_mesh_id_list[i] != MESH_ID_FSE) {
				mesh = mesh_scan_tile_mesh_id_list[i++];
				if (mesh == MESH_ID_FST || mesh == MESH_ID_FSS) {
					continue;
				} else if (mesh == MESH_ID_FSP || /* mesh == MESH_ID_FED */ mesh == MESH_ID_FTE) {
					reorder_mesh_id_list[mesh_id_idx++] = mesh; // meta-data header
				} else /* if (mesh != MESH_ID_FED) */ {
					if (reorder_id_map[mesh] == -1) {
						reorder_id_map[mesh] = reorder_mesh;
						reorder_mesh++;

						for (int l = 0; l < 4; l++) {
							reorder_mesh_slice_tbl[mesh_idx++] = isrc_x_mesh_tbl[mesh][l];
							reorder_mesh_slice_tbl[mesh_idx++] = isrc_y_mesh_tbl[mesh][l];
							reorder_mesh_slice_tbl[mesh_idx++] = idst_x_mesh_tbl[mesh][l];
							reorder_mesh_slice_tbl[mesh_idx++] = idst_y_mesh_tbl[mesh][l];
						}
					}

					reorder_mesh_id_list[mesh_id_idx++] = reorder_id_map[mesh];
				}
			}

			// slice end ID
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];
		}
	}

	// frame end ID
	reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];

	*reorder_mesh_tbl_entry_num = mesh_idx;
	*reorder_mesh_id_list_entry_num = mesh_id_idx;

	free(reorder_id_map);

	return 0;
}

struct mesh_param {
	CVI_U32 width;          // dst frame's width
	CVI_U32 height;         // dst frame's height
	void *mesh_id_addr;
	void *mesh_tbl_addr;
	CVI_U64 mesh_tbl_phy_addr;
	CVI_U8 slice_num_w;
	CVI_U8 slice_num_h;
	CVI_U16 mesh_num;
};

static bool is_rect_overlap(POINT_S l1, POINT_S r1, POINT_S l2, POINT_S r2)
{
	// If one rectangle is on left side of other
	if (l1.s32X > r2.s32X || l2.s32X > r1.s32X)
		return false;

	// If one rectangle is above other
	if (l1.s32Y > r2.s32Y || l2.s32Y > r1.s32Y)
		return false;

	return true;
}

static void _generate_mesh_id(struct mesh_param *param, POINT_S dst_mesh_tbl[][4])
{
	const int NUM_X_LINE_A_SLICE = DIV_UP(param->width, param->slice_num_w);
	const int NUM_Y_LINE_A_SLICE = ALIGN(DIV_UP(param->height, param->slice_num_h), NUMBER_Y_LINE_A_SUBTILE);
	CVI_U16 *mesh_id = param->mesh_id_addr;
	CVI_U16 slice_idx_x, slice_idx_y;
	CVI_U32 id_idx = 0;

	mesh_id[id_idx++] = MESH_ID_FST;
	for (slice_idx_y = 0; slice_idx_y < param->slice_num_h; slice_idx_y++) {
		int src_y = CLIP((slice_idx_y * NUM_Y_LINE_A_SLICE), 0, param->height);
		int dst_y = CLIP((src_y + NUM_Y_LINE_A_SLICE), 0, param->height);
		for (slice_idx_x = 0; slice_idx_x < param->slice_num_w; slice_idx_x++) {
			int src_x = CLIP((slice_idx_x * NUM_X_LINE_A_SLICE), 0, param->width);
			int dst_x = CLIP((src_x + NUM_X_LINE_A_SLICE), 0, param->width);

			mesh_id[id_idx++] = MESH_ID_FSS;
			mesh_id[id_idx++] = param->mesh_tbl_phy_addr & 0x0fff;
			mesh_id[id_idx++] = (param->mesh_tbl_phy_addr >> 12) & 0x0fff;
			mesh_id[id_idx++] = (param->mesh_tbl_phy_addr >> 24) & 0x0fff;
			mesh_id[id_idx++] = (param->mesh_tbl_phy_addr >> 36) & 0x000f;
			mesh_id[id_idx++] = src_x;
			mesh_id[id_idx++] = (dst_x - src_x);
			mesh_id[id_idx++] = src_y;
			mesh_id[id_idx++] = (dst_y - src_y);

			for (int y = src_y; y < dst_y; y += NUMBER_Y_LINE_A_SUBTILE) {
				POINT_S l1, r1, l2, r2;

				l1.s32X = src_x;
				l1.s32Y = y;
				r1.s32X = dst_x;
				r1.s32Y = y + NUMBER_Y_LINE_A_SUBTILE;
				mesh_id[id_idx++] = MESH_ID_FSP;
				for (int m = 0; m < param->mesh_num; ++m) {
					// To reduce time consumption
					// assumption: dst mesh is ordered (left->right, up->down)
					if (dst_mesh_tbl[m][3].s32Y < l1.s32Y) continue;
					if (dst_mesh_tbl[m][0].s32Y > r1.s32Y) break;

					l2.s32X = dst_mesh_tbl[m][0].s32X;
					l2.s32Y = dst_mesh_tbl[m][0].s32Y;
					r2.s32X = dst_mesh_tbl[m][3].s32X;
					r2.s32Y = dst_mesh_tbl[m][3].s32Y;
					if (is_rect_overlap(l1, r1, l2, r2))
						mesh_id[id_idx++] = m;
				}
				mesh_id[id_idx++] = MESH_ID_FTE;
			}
			mesh_id[id_idx++] = MESH_ID_FSE;
		}
	}
	mesh_id[id_idx++] = MESH_ID_FED;

	for (int i = 0; i < 32; ++i)
		mesh_id[id_idx++] = 0x00;
}

/**
 *  generate_mesh_on_faces: generate mesh based on the nodes describing the faces.
 *
 * @param param: mesh parameters.
 * @param pstAttr: affine's attributes
 */
static void generate_mesh_on_faces(struct mesh_param *param, const AFFINE_ATTR_S *pstAttr)
{
	const CVI_U8 w_knot_num = param->width/pstAttr->stDestSize.u32Width + 1;
	const CVI_U8 h_knot_num = param->height/pstAttr->stDestSize.u32Height + 1;
	// Limit slice's width/height to avoid unnecessary DRAM write(by bg-color)
	CVI_U16 width_slice = 0, height_slice = 0;
	CVI_U16 i, j, x, y, knot_idx;
	POINT_S dst_knot_tbl[w_knot_num * h_knot_num];
	POINT_S dst_mesh_tbl[param->mesh_num][4];
	CVI_U16 tbl_idx = 0;
	CVI_U32 *mesh_tbl = param->mesh_tbl_addr;

	// generate node
	for (j = 0; j < h_knot_num; ++j) {
		y = j * pstAttr->stDestSize.u32Height;
		for (i = 0; i < w_knot_num; ++i) {
			knot_idx = j * w_knot_num + i;
			x = i * pstAttr->stDestSize.u32Width;
			dst_knot_tbl[knot_idx].s32X = x;
			dst_knot_tbl[knot_idx].s32Y = y;
		}
	}

	// map node to each mesh
	for (i = 0; i < param->mesh_num; ++i) {
		knot_idx = i + (i / (param->width/pstAttr->stDestSize.u32Width));    // there is 1 more node than mesh each row

		dst_mesh_tbl[i][0] =  dst_knot_tbl[knot_idx];
		dst_mesh_tbl[i][1] =  dst_knot_tbl[knot_idx + 1];
		dst_mesh_tbl[i][2] =  dst_knot_tbl[knot_idx + w_knot_num];
		dst_mesh_tbl[i][3] =  dst_knot_tbl[knot_idx + w_knot_num + 1];

		if (dst_mesh_tbl[i][1].s32X > width_slice) width_slice = dst_mesh_tbl[i][1].s32X;
		if (dst_mesh_tbl[i][2].s32Y > height_slice) height_slice = dst_mesh_tbl[i][2].s32Y;

		for (j = 0; j < 4; ++j) {
			mesh_tbl[tbl_idx++] = pstAttr->astRegionAttr[i][j].x * (double)(1 << DEWARP_COORD_NBITS);
			mesh_tbl[tbl_idx++] = pstAttr->astRegionAttr[i][j].y * (double)(1 << DEWARP_COORD_NBITS);
			mesh_tbl[tbl_idx++] = dst_mesh_tbl[i][j].s32X * (double)(1 << DEWARP_COORD_NBITS);
			mesh_tbl[tbl_idx++] = dst_mesh_tbl[i][j].s32Y * (double)(1 << DEWARP_COORD_NBITS);
		}
	}

	param->width = width_slice;
	param->height = height_slice;
	_generate_mesh_id(param, dst_mesh_tbl);
}

void mesh_gen_affine(SIZE_S in_size, SIZE_S out_size, const AFFINE_ATTR_S *pstAffineAttr
	, uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
	const int MAX_MESH_NUM_A_TILE = 4;
	int X_TILE_NUMBER, Y_TILE_NUMBER;
	int owidth, oheight;
	int Y_SUBTILE_NUMBER;
	CVI_U32 mesh_id_size, mesh_tbl_size;
	CVI_U64 mesh_id_phy_addr, mesh_tbl_phy_addr;

	owidth = out_size.u32Width;
	oheight = out_size.u32Height;
	X_TILE_NUMBER = DIV_UP(in_size.u32Width, 122);
	Y_TILE_NUMBER = 8;

	// calculate mesh_id/mesh_tbl's size in bytes.
	Y_SUBTILE_NUMBER = ceil((float)out_size.u32Height / (float)NUMBER_Y_LINE_A_SUBTILE);
	mesh_tbl_size = X_TILE_NUMBER * Y_TILE_NUMBER * 16 * sizeof(CVI_U32);
	mesh_id_size = MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER * sizeof(CVI_U16);

	// Decide the position of mesh in memory.
	mesh_id_phy_addr = mesh_phy_addr;
	mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
		     , mesh_id_phy_addr, mesh_tbl_phy_addr);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

	struct mesh_param param;

	param.width = owidth;
	param.height = oheight;
	param.mesh_id_addr = mesh_vir_addr;
	param.mesh_tbl_addr = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);;
	param.mesh_tbl_phy_addr = mesh_tbl_phy_addr;
	param.slice_num_w = 1;
	param.slice_num_h = 1;
	param.mesh_num = pstAffineAttr->u32RegionNum;
	generate_mesh_on_faces(&param, pstAffineAttr);

}


/* For fisheye
 *
 */

//#define M_PI   3.14159265358979323846264338327950288
#define minmax(a,b,c)  (((a)<(b))? (b):((a)>(c))? (c):(a))
#define UI_CTRL_VALUE_CENTER 180	// range from -16 ~ 16,  "16" is the center of value 0 ~32

typedef enum _PROJECTION_MODE {
	PROJECTION_PANORAMA_360,
	PROJECTION_PANORAMA_180,
	PROJECTION_REGION,
	PROJECTION_LDC,
} PROJECTION_MODE;

typedef struct _FORMAT_8BITS_RGB {
	unsigned R;
	unsigned G;
	unsigned B;
} FORMAT_8BITS_RGB;

typedef struct _COORDINATE2D {
	double xcor;
	double ycor;
}COORDINATE2D;

typedef struct _MESH_STRUCT {
	COORDINATE2D knot[4];

	int idx;
}MESH_STRUCT;

#define MAX_REGION_NUM 5
#define MESHINFO_DEBUG 0
#define MAX_FRAME_MESH_NUM 4096

#define UI_CTRL_VALUE_CENTER 180	// range from -16 ~ 16,  "16" is the center of value 0 ~32

typedef struct _FISHEYE_ATTR {

	bool Enable;					// dewarp engine on/off
	bool BgEnable;					// given background color enable.

	int RgnNum;						// dewarp Region Number.
	int TotalMeshNum;				// total mesh numbes
	int OutW_disp;					// output display frame width
	int OutH_disp;					// output display frame height
	int InCenterX;					// input fisheye center position X
	int InCenterY;					// input fisheye center position Y
	int InRadius;					// input fisheye radius in pixels.

	double Hoffset;					// fish-eye image center horizontal offset.
	double VOffset;					// fish-eye image center vertical offset.
	double TCoef;					// KeyStone Corection coefficient.
	double FStrength;				// Fan Correction coefficient.

	USAGE_MODE UsageMode;			// Panorama.360, Panorama.180, Mode1~Mode7. total = 9 for now.
	FISHEYE_MOUNT_MODE_E MntMode;		// CEILING/ FLOOR/ WALL
	FORMAT_8BITS_RGB BgColor;		// give background color RGB.

	//MESH_STRUCT DstRgnMeshInfo[16384];	// frame-based dst mesh info.  maximum num=128*28 = 16384 meshes.
	//MESH_STRUCT SrcRgnMeshInfo[16384];	// frame-based src mesh info.

	MESH_STRUCT DstRgnMeshInfo[MAX_FRAME_MESH_NUM];	// frame-based dst mesh info.  maximum num=128*28 = 16384 meshes.
	MESH_STRUCT SrcRgnMeshInfo[MAX_FRAME_MESH_NUM];	// frame-based src mesh info.

	int SliceX_Num;		// How many tile is sliced in a frame horizontally.
	int SliceY_Num;		// How many tile is sliced in a frame vertically.

	int rotate_index;
} FISHEYE_ATTR;

typedef struct _FISHEYE_REGION_ATTR {
	int RgnIndex;		// region index
	float InRadius;		// For Panorama.360 mode, inner radius.
	float OutRadius;	// For Panorama.360 mode, outer radius.
	int Pan;		// Region PTZ-Pan.
	int Tilt;		// Region PTZ-Tilt.
	int HorZoom;		// adjust horizontal zoom in Region PTZ.
	int VerZoom;		// adjust vertical zoom in Region PTZ.
	int OutX;			// region initial position x on display frame.
	int OutY;			// region initial position y on display frame.
	int OutW;			// region width.
	int OutH;			// region height.
	int MeshHor;		// mesh counts horizontal
	int MeshVer;		// mesh counts vertical

	// to give region default view center
	double ThetaX;		// User set rotation angle around X-axis, create angle between vector to Z-axis. (start position @ [0,0,1])
	double ThetaY;		// User set rotation angle around Y-axis,
	double ThetaZ;		// User set rotation angle around Z-axis,

	double ZoomH;		// User Set for Region
	double ZoomV;		// User Set for Region
	int PanEnd;		// For Panorama Case Only
	bool RegionValid;	// label valid/ invalid for each region

	MESH_STRUCT DstRgnMeshInfo[MAX_FRAME_MESH_NUM];	// initial buffer to store destination mesh info. max: 32*32
	MESH_STRUCT SrcRgnMeshInfo[MAX_FRAME_MESH_NUM];	// initial buffer to store source mesh info
	PROJECTION_MODE ViewMode;			// projection method. ex: Panorama.360, Panorama.180, Region
} FISHEYE_REGION_ATTR;

typedef struct Vector3D {
	double	x;
	double	y;
	double	z;
} Vector3D;

typedef struct Vector2D {
	double	x;
	double	y;
} Vector2D;

typedef struct RotMatrix3D {
	double	coef[4][4];
} RotMatrix3D;


void Normalize3D(Vector3D* v)
{
	double x3d, y3d, z3d, d;

	x3d = v->x;
	y3d = v->y;
	z3d = v->z;
	d = sqrt(x3d * x3d + y3d * y3d + z3d * z3d);
	v->x = x3d / d;
	v->y = y3d / d;
	v->z = z3d / d;
}

void Equidistant_Panorama(Vector3D* v3d, Vector2D* v2d, double f)
{
	double rd, theta;

	// calculate theta from the angle between (0,0,1) and (x3d, y3d, z3d)
	//theta = acos(v3d->z); //bastian

	theta = v3d->z;

	if (theta == 0.0) {
		theta += 1e-64;
	}

#if 0
	// calculate rd = f * theta
	rd = f * (theta / (M_PI / 2));
	phy = atan(rd / f);

	// calculate new distorted vector (x,y,z) = (0,0,1) * (1-rd/ri) + (x3d, y3d, z3d) * (rd/ri)
	Vector3D v3d_p;
	v3d_p.x = v3d->x * phy / theta;
	v3d_p.y = v3d->y * phy / theta;
	v3d_p.z = v3d->z * phy / theta + (1 - phy / theta);

	// calculate the target point (xi, yi) on image at z = f
	v2d->x = (f / v3d_p.z) * v3d_p.x;
	v2d->y = (f / v3d_p.z) * v3d_p.y;

#else
	// calculate rd = f * theta
	// assume: fisheye image 2D' radius rd
	// if it's equidistant: r = focal*incident_angle
	// max incident angle = PI/2
	rd = f * (theta / (M_PI / 2));

	// ( 0,0 ) & (v3d->x, v3d->y): find equation y = ax + b
	// a = v3d.y/v3d.x
	// if incident angle from space ( x, y ),
	double L_a = v3d->y / v3d->x;

	v2d->x = (v3d->x >= 0) ? (-sqrt((rd * rd) / (L_a * L_a + 1))) : sqrt((rd * rd) / (L_a * L_a + 1));
	v2d->y = L_a * v2d->x;

#endif
}

void Equidistant(Vector3D* v3d, Vector2D* v2d, double f)
{
	double rd, ri, theta, phy;

	Normalize3D(v3d);

	// calculate theta from the angle between (0,0,1) and (x3d, y3d, z3d)
	theta = acos(v3d->z);

	// calculate ri = f tan(theta)
	ri = f * tan(theta);
	if (theta == 0.0) {
		ri += 1e-64;
		theta += 1e-64;
	}

	// calculate rd = f * theta
	rd = f * (theta / (M_PI / 2));
	phy = atan(rd / f);

	// calculate new distorted vector (x,y,z) = (0,0,1) * (1-rd/ri) + (x3d, y3d, z3d) * (rd/ri)
	Vector3D v3d_p;

	v3d_p.x = v3d->x * phy / theta;
	v3d_p.y = v3d->y * phy / theta;
	v3d_p.z = v3d->z * phy / theta + (1 - phy / theta);

	// calculate the target point (xi, yi) on image at z = f
	v2d->x = (f / v3d_p.z) * v3d_p.x;
	v2d->y = (f / v3d_p.z) * v3d_p.y;
}

void Rotate3D(Vector3D* v3d, RotMatrix3D* mat)
{
	//int i, j;
	Vector3D v1;

	v1.x = mat->coef[0][0] * v3d->x + mat->coef[0][1] * v3d->y +
		mat->coef[0][2] * v3d->z + mat->coef[0][3];
	v1.y = mat->coef[1][0] * v3d->x + mat->coef[1][1] * v3d->y +
		mat->coef[1][2] * v3d->z + mat->coef[1][3];
	v1.z = mat->coef[2][0] * v3d->x + mat->coef[2][1] * v3d->y +
		mat->coef[2][2] * v3d->z + mat->coef[2][3];

	v3d->x = v1.x;
	v3d->y = v1.y;
	v3d->z = v1.z;
}

void GenRotMatrix3D(Vector3D* v3d, RotMatrix3D* mat)
{
	// generate the rotation matrix to
	// rotate vector [0,0,1] to v3d

#if 1
	double phy1, phy2;
	Vector3D v1, v2;

	// 1. rotate v3d along X axis to have the vector lies in X-Z plane, i.e. y=0 and x is postive
	// cos(phy1) * v3d->y + sin(phy1) * v3d->z = 0;
	// solve phy1
	// v3d->y * cos(phy1) = -v3d->z * sin(phy1)
	// -v3d->y / v3d->z = tan(phy1)
	// phy1 = atan(-v3d->y/v3d->z)
	// if(v3d->x <0)  phy1 += MI_PI;
	// update new v3d by Rx(phy1)


	//從最後已經作完乘法的旋轉矩陣來看，如果要分別求得 θy, θx, θz ，其計算方式為：
	//θy = arctan(r13 / r33)
	//θx = arcsin(-r23)
	//θz = arctan(r21 / r22)

	phy1 = atan(-v3d->y / v3d->z);

	v1.x = v3d->x;
	v1.y = v3d->y * cos(phy1) + v3d->z * sin(phy1);
	v1.z = -v3d->y * sin(phy1) + v3d->z * cos(phy1);

	// 2. rotate v3d along Y axis to have the vector lies in Z axis, i.e. x=0, y=0 and z is positive
	// v3d->x * cos(phy2) - v3d->z * sin(phy2) = 0
	// v3d->x / v3d->z = tan(phy2)
	// phy2 = atan(v3d->x / v3d->z)
	// v3d->z = v3d->x * sin(phy2) + v3d->z*cos(phy2)
	// if (v3d->z < 0)  phy2 += M_PI;
	phy2 = atan(v1.x / v1.z);
	v2.z = v1.x * sin(phy2) + v1.z * cos(phy2);
	if (v2.z < 0)	phy2 += M_PI;

	v2.x = v1.x * cos(phy2) - v1.z * sin(phy2);
	v2.y = v1.y;
	v2.z = v1.x * sin(phy2) + v1.z * cos(phy2);

	// do the reverse rotation
	// Rotation matrix = Rx(-phy1)*Ry(-phy2)
	mat->coef[0][0] = cos(-phy2);
	mat->coef[0][1] = 0;
	mat->coef[0][2] = -sin(-phy2);
	mat->coef[0][3] = 0;
	mat->coef[1][0] = sin(-phy1) * sin(-phy2);
	mat->coef[1][1] = cos(-phy1);
	mat->coef[1][2] = sin(-phy1) * cos(-phy2);
	mat->coef[1][3] = 0;
	mat->coef[2][0] = cos(-phy1) * sin(-phy2);
	mat->coef[2][1] = -sin(-phy1);
	mat->coef[2][2] = cos(-phy1) * cos(-phy2);
	mat->coef[2][3] = 0;
	mat->coef[3][0] = 0;
	mat->coef[3][1] = 0;
	mat->coef[3][2] = 0;
	mat->coef[3][3] = 1;
#else
	mat->coef[0][0] = cos(θy) * cos(θz) - sin(θx) * sin(θy) * sin(θz);
	mat->coef[0][1] = -cos(θx) * sin(θz);
	mat->coef[0][2] = sin(θy) * cos(θz) + sin(θx) * cos(θy) * sin(θz);
	mat->coef[0][3] = 0;
	mat->coef[1][0] = cos(θy) * sin(θz) + sin(θx) * sin(θy) * cos(θz);
	mat->coef[1][1] = cos(θx) * cos(θz);
	mat->coef[1][2] = sin(θy) * sin(θz) - sin(θx) * cos(θy) * cos(θz);
	mat->coef[1][3] = 0;
	mat->coef[2][0] = -cos(θx) * sin(θy);
	mat->coef[2][1] = sin(θx);
	mat->coef[2][2] = cos(θx) * cos(θy);
	mat->coef[2][3] = 0;
	mat->coef[3][0] = 0;
	mat->coef[3][1] = 0;
	mat->coef[3][2] = 0;
	mat->coef[3][3] = 1;
#endif
}

void GenRotMatrix3D_YXZ(RotMatrix3D* mat, FISHEYE_REGION_ATTR* FISHEYE_REGION_IDX)
{
	// This Rotation Matrix Order = R = Rz*Rx*Ry
	// rotation order = θy -> θx -> θz
	//_LOAD_REGION_CONFIG;
	// initital position
	double tmp_phy_x = FISHEYE_REGION_IDX->ThetaX;	//phy_x;
	double tmp_phy_y = FISHEYE_REGION_IDX->ThetaY;	//phy_y;	// Not Used for Now.
	double tmp_phy_z = FISHEYE_REGION_IDX->ThetaZ;	//phy_z;

	//_LOAD_REGION_CONFIG(FISHEYE_CONFIG, FISHEYE_REGION);
	// UI Control
	double ctrl_tilt, ctrl_pan;// = minmax(FISHEYE_REGION_IDX[rgn_idx].Tilt - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
	//double ctrl_pan  = minmax(FISHEYE_REGION_IDX[rgn_idx].Pan - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);

	if (FISHEYE_REGION_IDX->ViewMode == PROJECTION_REGION)
	{
		ctrl_tilt = minmax(FISHEYE_REGION_IDX->Tilt - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
		ctrl_pan  = minmax(FISHEYE_REGION_IDX->Pan  - UI_CTRL_VALUE_CENTER, -UI_CTRL_VALUE_CENTER, UI_CTRL_VALUE_CENTER);
	}
	else
	{
		// not used in panorama case
		ctrl_pan  = 0;
		ctrl_tilt = 0;
	}

	tmp_phy_x += (ctrl_tilt * M_PI / 2)/(2* UI_CTRL_VALUE_CENTER);
	tmp_phy_y += 0;
	tmp_phy_z += (ctrl_pan * M_PI)/(2* UI_CTRL_VALUE_CENTER);

	mat->coef[0][0] = cos(tmp_phy_y) * cos(tmp_phy_z) - sin(tmp_phy_x) * sin(tmp_phy_y) * sin(tmp_phy_z);
	mat->coef[0][1] = -cos(tmp_phy_x) * sin(tmp_phy_z);
	mat->coef[0][2] = sin(tmp_phy_y) * cos(tmp_phy_z) + sin(tmp_phy_x) * cos(tmp_phy_y) * sin(tmp_phy_z);
	mat->coef[0][3] = 0;
	mat->coef[1][0] = cos(tmp_phy_y) * sin(tmp_phy_z) + sin(tmp_phy_x) * sin(tmp_phy_y) * cos(tmp_phy_z);
	mat->coef[1][1] = cos(tmp_phy_x) * cos(tmp_phy_z);
	mat->coef[1][2] = sin(tmp_phy_y) * sin(tmp_phy_z) - sin(tmp_phy_x) * cos(tmp_phy_y) * cos(tmp_phy_z);
	mat->coef[1][3] = 0;
	mat->coef[2][0] = -cos(tmp_phy_x) * sin(tmp_phy_y);
	mat->coef[2][1] = sin(tmp_phy_x);
	mat->coef[2][2] = cos(tmp_phy_x) * cos(tmp_phy_y);
	mat->coef[2][3] = 0;
	mat->coef[3][0] = 0;
	mat->coef[3][1] = 0;
	mat->coef[3][2] = 0;
	mat->coef[3][3] = 1;
}

void _do_mesh_rotate(int rotate_index, int view_h, int view_w, double xin, double yin, double *xout, double *yout)
{
	double RMATRIX[2][2];

	if (rotate_index == 0)			//00 degrees
	{
		RMATRIX[0][0] = 1;
		RMATRIX[0][1] = 0;
		RMATRIX[1][0] = 0;
		RMATRIX[1][1] = 1;
	}
	else if (rotate_index == 1)			//-90 degrees
	{
		RMATRIX[0][0] = 0;
		RMATRIX[0][1] = 1;
		RMATRIX[1][0] = -1;
		RMATRIX[1][1] = 0;
	}
	else if (rotate_index == 2)				// + 90 degrees
	{
		RMATRIX[0][0] = -1;
		RMATRIX[0][1] = 0;
		RMATRIX[1][0] = 0;
		RMATRIX[1][1] = -1;
	}
	else
	{
		RMATRIX[0][0] = 0;
		RMATRIX[0][1] = -1;
		RMATRIX[1][0] = 1;
		RMATRIX[1][1] = 0;
	}

	*xout = RMATRIX[0][0] * xin + RMATRIX[1][0] * yin;
	*yout = RMATRIX[0][1] * xin + RMATRIX[1][1] * yin;


#if 0
	//printf("view_h = %d\n", view_h);
	//printf("view_w = %d\n", view_w);

	printf(" RMATRIX[0][0] = %f\n", RMATRIX[0][0]);
	printf(" RMATRIX[0][1] = %f\n", RMATRIX[0][1]);
	printf(" RMATRIX[1][0] = %f\n", RMATRIX[1][0]);
	printf(" RMATRIX[1][1] = %f\n", RMATRIX[1][1]);

	printf("xin = %f\n", xin);
	printf("yin = %f\n", yin);
	printf("xout = %f\n", xout);
	printf("yout = %f\n", yout);
	system("pause");
#endif

	if (rotate_index == 0) {
		*xout = *xout;
		*yout = *yout;
	} else if (rotate_index == 1) {
		*xout = *xout + (view_h - 1);
		*yout = *yout;
	} else if (rotate_index == 2) {
		*xout = *xout + (view_w - 1);
		*yout = *yout + (view_h - 1);
	} else {
		*xout = *xout + 0;
		*yout = *yout + (view_w - 1);
	}
}

void _get_frame_mesh_list(FISHEYE_ATTR* FISHEYE_CONFIG, FISHEYE_REGION_ATTR* FISHEYE_REGION)
{
	// pack all regions' mesh info, including src & dst.
	int rgnNum = FISHEYE_CONFIG->RgnNum;
	int meshNumRgn;
	int frameMeshIdx = 0;
	int rotate_index = FISHEYE_CONFIG->rotate_index;
	int view_w = FISHEYE_CONFIG->OutW_disp;
	int view_h = FISHEYE_CONFIG->OutH_disp;

	for (int i = 0; i < rgnNum; i++) {
		if (FISHEYE_REGION[i].RegionValid == 1) {
			meshNumRgn = (FISHEYE_REGION[i].MeshHor * FISHEYE_REGION[i].MeshVer);

			// go through each region loop
			for (int meshidx = 0; meshidx < meshNumRgn; meshidx++) {
				// each mesh has 4 knots
				for (int knotidx = 0; knotidx < 4; knotidx++) {
					// do rotaion:
					double x_src = FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].xcor;
					double y_src = FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].ycor;
					double x_dst = FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].xcor;
					double y_dst = FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].ycor;

					double x_dst_out, y_dst_out;
					_do_mesh_rotate(rotate_index, view_h, view_w, x_dst, y_dst, &x_dst_out, &y_dst_out);

					FISHEYE_CONFIG->DstRgnMeshInfo[frameMeshIdx].knot[knotidx].xcor = x_dst_out; //FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].xcor;
					FISHEYE_CONFIG->DstRgnMeshInfo[frameMeshIdx].knot[knotidx].ycor = y_dst_out; //FISHEYE_REGION[i].DstRgnMeshInfo[meshidx].knot[knotidx].ycor;
					FISHEYE_CONFIG->SrcRgnMeshInfo[frameMeshIdx].knot[knotidx].xcor = x_src; //FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].xcor;
					FISHEYE_CONFIG->SrcRgnMeshInfo[frameMeshIdx].knot[knotidx].ycor = y_src; //FISHEYE_REGION[i].SrcRgnMeshInfo[meshidx].knot[knotidx].ycor;
				}
				frameMeshIdx += 1;
			}
		}
	}

	// update mesh index number
	FISHEYE_CONFIG->TotalMeshNum = frameMeshIdx;
}

void _Panorama180View2(FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, FISHEYE_MOUNT_MODE_E MOUNT, double x0, double y0, double r)
{
	if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_PANORAMA_180)
		return;
	if (MOUNT != FISHEYE_WALL_MOUNT) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "mount_mode(%d) not supported in Panorama180\n", MOUNT);
		return;
	}

	int view_w = FISHEYE_REGION[rgn_idx].OutW;
	int view_h = FISHEYE_REGION[rgn_idx].OutH;
	int tot_mesh_cnt = FISHEYE_REGION[rgn_idx].MeshHor * FISHEYE_REGION[rgn_idx].MeshVer;

	//printf("view_w = %d, view_h = %d, \n", view_w, view_h);
	//printf("mesh_horcnt = %d,\n", mesh_horcnt);
	//printf("mesh_vercnt = %d,\n", mesh_vercnt);


	// UI PARAMETERS
	int _UI_horViewOffset = FISHEYE_REGION[rgn_idx].Pan;		// value range = 0 ~ 360, => -180 ~ 0 ~ +180
	int _UI_verViewOffset = FISHEYE_REGION[rgn_idx].Tilt;		// value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
	int _UI_horViewRange  = FISHEYE_REGION[rgn_idx].ZoomH;		// value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
	int _UI_verViewRange  = FISHEYE_REGION[rgn_idx].ZoomV;		// value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90

	_UI_verViewRange = (_UI_verViewRange == 4095) ? 4096 : _UI_verViewRange;
	_UI_horViewRange = (_UI_horViewRange == 4095) ? 4096 : _UI_horViewRange;

	//printf("_UI_horViewOffset = %d,\n", _UI_horViewOffset);
	//printf("_UI_verViewOffset = %d,\n", _UI_verViewOffset);
	//printf("_UI_horViewRange = %d,\n", _UI_horViewRange);
	//printf("_UI_verViewRange = %d,\n", _UI_verViewRange);

	// calculate default view range:
	double view_range_ver_degs = (double)_UI_verViewRange * 90 / 4096;
	double view_range_hor_degs = (double)_UI_horViewRange * 90 / 4096;
	double va_ver_degs = view_range_ver_degs;
	double va_hor_degs = view_range_hor_degs;

	// calculate offsets
	double va_hor_offset = ((double)_UI_horViewOffset - 180) * 90 / 360;
	double va_ver_offset = ((double)_UI_verViewOffset - 180) * 90 / 360;

	//printf("va_hor_offset = %f,\n", va_hor_offset);
	//printf("va_ver_offset = %f,\n", va_ver_offset);

	// Offset to shift view angle
	double va_ver_start = minmax(90 - va_ver_degs + va_ver_offset,  0,  90);
	double va_ver_end   = minmax(90 + va_ver_degs + va_ver_offset, 90, 180);
	double va_hor_start = minmax(90 - va_hor_degs + va_hor_offset,  0,  90);
	double va_hor_end   = minmax(90 + va_hor_degs + va_hor_offset, 90, 180);

	//printf("va_ver_start = %f,\n", va_ver_start);
	//printf("va_ver_end = %f,\n", va_ver_end);
	//printf("va_hor_start = %f,\n", va_hor_start);
	//printf("va_hor_end = %f,\n", va_hor_end);

	//system("pause");

	RotMatrix3D mat0;

	Vector3D pix3d;
	Vector2D dist2d;
	int X_offset = FISHEYE_REGION[rgn_idx].OutX + FISHEYE_REGION[rgn_idx].OutW / 2;
	int Y_offset = FISHEYE_REGION[rgn_idx].OutY + FISHEYE_REGION[rgn_idx].OutH / 2;

	for (int i = 0; i < tot_mesh_cnt; i++)
	{
		//printf("i = %d,", i);
		// each mesh has 4 knots
		for (int knotidx = 0; knotidx < 4; knotidx++)
		{
			double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
			double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;
			//double z = r;

			// initial pseudo plane cooridinates as vp3d
			//double theta_hor_cur = va_hor_start + ((2 * va_hor_degs * (view_w / 2 - x)) / (double)view_w);
			double theta_hor_cur = va_hor_start + (((va_hor_end - va_hor_start) * (view_w / 2 - x)) / (double)view_w);
			double theta_hor_cur_rad = M_PI / 2 - (theta_hor_cur * M_PI) / 180;		//θx

			double theta_ver_cur = va_ver_start + (((va_ver_end - va_ver_start) * (view_h / 2 - y)) / (double)view_h);
			double theta_ver_cur_rad = M_PI / 2 - (theta_ver_cur * M_PI) / 180;		//θy

			//if (knotidx == 0)
			//{
			//	printf("(x,y)=(%f,%f) FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor = %f, ", x, y, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor);
			//	printf("hor_deg = %f, ver_deg = %f,\n\r",theta_hor_cur, theta_ver_cur);
			//	//printf("hor_rad = %f,  ver_rad = %f, \n\r", theta_hor_cur_rad, theta_ver_cur_rad);
			//}

			double theta_x = theta_hor_cur_rad;
			double theta_y = theta_ver_cur_rad;
			double theta_z = (M_PI / 2);

			FISHEYE_REGION[rgn_idx].ThetaX = theta_x;
			FISHEYE_REGION[rgn_idx].ThetaY = theta_y;
			FISHEYE_REGION[rgn_idx].ThetaZ = theta_z;

			GenRotMatrix3D_YXZ(&mat0, &FISHEYE_REGION[rgn_idx]);

			pix3d.x = 0;
			pix3d.y = 0;
			pix3d.z = 1;

			Rotate3D(&pix3d, &mat0);

			// generate new 3D vector thru rotated pixel
			//Normalize3D(&pix3d);

			// generate 2D location on distorted image
			//Equidistant_Panorama(&pix3d, &dist2d, r);
			Equidistant(&pix3d, &dist2d, r);

			// update source mesh-info here
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;

			//printf("(x2d)(%f,%f),\n\r",dist2d.x + x0, dist2d.y + y0);

		}
	}
}

void _Panorama360View2( FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, FISHEYE_MOUNT_MODE_E MOUNT, double x0, double y0, double r)
{
	if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_PANORAMA_360)
		return;
	if ((MOUNT != FISHEYE_CEILING_MOUNT) && (MOUNT != FISHEYE_DESKTOP_MOUNT)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "mount_mode(%d) not supported in Panorama360\n", MOUNT);
		return;
	}

	int view_w = FISHEYE_REGION[rgn_idx].OutW;
	int view_h = FISHEYE_REGION[rgn_idx].OutH;
	int tot_mesh_cnt = FISHEYE_REGION[rgn_idx].MeshHor * FISHEYE_REGION[rgn_idx].MeshVer;

	//Vector2D vp2d;
	//Vector3D vp3d;
	//RotMatrix3D mat0;

	Vector3D pix3d;
	Vector2D dist2d;

	// UI PARAMETERS
	//float _UI_start_angle		= FISHEYE_REGION[rgn_idx].Pan;					// value from 0=360, 0 ~ 356 is the range of adjustment.
	float _UI_view_offset		= FISHEYE_REGION[rgn_idx].Tilt;					// value = 0 ~ 360, center = 180 ( original ), OR create offset to shift OutRadius & InRadius ( = adjust theta angle in our implementation )
	float _UI_inradius		= FISHEYE_REGION[rgn_idx].InRadius;				// value = 0 ~ 4095
	float _UI_outradius		= FISHEYE_REGION[rgn_idx].OutRadius;				// value = 0 ~ 4095
	float _UI_zoom_outradius	= FISHEYE_REGION[rgn_idx].ZoomV;				// a ratio to zoom OutRadius length.

	_UI_inradius  = (_UI_inradius >= 4095)  ? 4096 : _UI_inradius;
	_UI_outradius = (_UI_outradius >= 4095) ? 4096 : _UI_outradius;
	_UI_zoom_outradius = (_UI_zoom_outradius >= 4095) ? 4096 : _UI_zoom_outradius;

	int start_angle_degrees  = FISHEYE_REGION[rgn_idx].Pan;
	int end_angle__degrees   = FISHEYE_REGION[rgn_idx].PanEnd;


	float raw_outradius_pxl = (_UI_outradius * r )/ 4096;
	float raw_inradius_pxl  = (_UI_inradius * r )/ 4096;
	float radiusOffset      = (raw_outradius_pxl * (_UI_view_offset - 180)) / 360;

	float inradius_pxl_final  = MIN(r, MAX(0, raw_inradius_pxl + radiusOffset));
	float outradius_pxl_final = MIN(r, MAX(inradius_pxl_final, raw_inradius_pxl + (MAX(0, (raw_outradius_pxl - raw_inradius_pxl)) * _UI_zoom_outradius) / 4096 + radiusOffset));

	//printf("r = %f,\n", r);

	//printf("raw_outradius_pxl = %f,\n", raw_outradius_pxl);
	//printf("raw_inradius_pxl = %f,\n", raw_inradius_pxl);
	//printf("radiusOffset = %f,\n", radiusOffset);
	//printf("inradius_pxl_final = %f,\n", inradius_pxl_final);
	//printf("outradius_pxl_final = %f,\n", outradius_pxl_final);

	float va_ver_end_rads   = inradius_pxl_final * M_PI / (2 * r);//_UI_inradius * M_PI / 1024;					// for Equidistant =>rd = f*theta, rd_max = f*PI/2 = r ( in code), f = 2r/PI. => theta = rd/f = rd*PI/2r
	float va_ver_start_rads = outradius_pxl_final * M_PI / (2 * r);//_UI_outradius * M_PI / 1024;
	float va_ver_rads = MIN( M_PI/2, va_ver_start_rads - va_ver_end_rads);

	//printf("va_ver_end_rads = %f,\n", va_ver_end_rads);
	//printf("va_ver_start_rads = %f,\n", va_ver_start_rads);
	//printf("va_ver_rads = %f,\n", va_ver_rads);


	//printf("( %d, %d, %d, %f, %f, %d,) \n", rgn_idx, _UI_start_angle, _UI_view_offset, _UI_inradius, _UI_outradius, _UI_zoom_outradius);
	//printf("start_angle_degrees = %d, end_angle__degrees = %d, va_ver_degs = %f, va_ver_start_degs = %f, \n\r", start_angle_degrees, end_angle__degrees, va_ver_rads, va_ver_start_rads);
	//system("pause");


	int total_angle = (360 + (end_angle__degrees - start_angle_degrees)) % 360;
	int half_w = view_w / 2;
	int half_h = view_h / 2;
	int X_offset = FISHEYE_REGION[rgn_idx].OutX + FISHEYE_REGION[rgn_idx].OutW / 2;
	int Y_offset = FISHEYE_REGION[rgn_idx].OutY + FISHEYE_REGION[rgn_idx].OutH / 2;

	for (int i = 0; i < tot_mesh_cnt; i++)
	{
		// each mesh has 4 knots
		for (int knotidx = 0; knotidx < 4; knotidx++)
		{
			double phi_degrees, phi_rad, theta_rv;

			double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor	- X_offset;
			double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor	- Y_offset;

			if(MOUNT == FISHEYE_DESKTOP_MOUNT) {
				phi_degrees = (total_angle * (half_w - x )) / (double)view_w;
				phi_rad = (((start_angle_degrees + phi_degrees) * M_PI)) / 180;
				//theta_rv = (((va_ver_degs * M_PI) / 180) * (y + (view_h / 2))) / (double)(view_h);
				theta_rv = ( va_ver_rads * (y + half_h)) / (double)(view_h);
			} else if (MOUNT == FISHEYE_CEILING_MOUNT) {
				phi_degrees = (total_angle * (half_w + x)) / (double)view_w;
				phi_rad = (((start_angle_degrees + phi_degrees) * M_PI)) / 180;
				//theta_rv = (((va_ver_degs * M_PI) / 180) * ((view_h / 2) - y)) / (double)(view_h);
				theta_rv = ( va_ver_rads * (half_h - y)) / (double)(view_h);
			}


			// 2D plane cooridnate to cylinder
			// rotation initial @ [x, y, z] = [1, 0, 0];
			double xc =  1 * cos(-phi_rad) + 0 * sin(-phi_rad);
			double yc = -1 * sin(-phi_rad) + 0 * cos(-phi_rad);
			double zc = theta_rv + va_ver_end_rads;

			//Rotate3D(&pix3d, &mat0);
			pix3d.x = xc;
			pix3d.y = yc;
			pix3d.z = zc;
			//Equidistant(&pix3d, &dist2d, r);

			Equidistant_Panorama(&pix3d, &dist2d, r);

			// update source mesh-info here
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;
		}
	}

}

void _LDC_View(FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, double x0, double y0)
{
	if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_LDC)
		return;

	int view_w = FISHEYE_REGION[rgn_idx].OutW;
	int view_h = FISHEYE_REGION[rgn_idx].OutH;
	int mesh_horcnt = FISHEYE_REGION[rgn_idx].MeshHor;
	int mesh_vercnt = FISHEYE_REGION[rgn_idx].MeshVer;


	// register:
	bool bAspect		= (bool)FISHEYE_REGION[0].ZoomV;
	int XYRatio		= minmax(FISHEYE_REGION[0].Pan, 0, 100);
	int XRatio		= minmax(FISHEYE_REGION[0].ThetaX, 0, 100);
	int YRatio		= minmax(FISHEYE_REGION[0].ThetaZ, 0, 100);
	int CenterXOffset	= minmax(FISHEYE_REGION[0].InRadius, -511, 511);
	int CenterYOffset	= minmax(FISHEYE_REGION[0].OutRadius, -511, 511);
	int DistortionRatio	= minmax(FISHEYE_REGION[0].PanEnd, -300, 500);


	double norm = sqrt((view_w / 2) * (view_w / 2) + (view_h / 2) * (view_h / 2));


	// internal link to register:
	double k = (double)DistortionRatio / 1000.0;
	double ud_gain = (1 + k * (((double)view_h / 2) * (double)view_h / 2) / norm / norm);
	double lr_gain = (1 + k * (((double)view_w / 2) * (double)view_w / 2) / norm / norm);


	double Aspect_gainX = MAX2(ud_gain, lr_gain);
	double Aspect_gainY = MAX2(ud_gain, lr_gain);


	Vector2D dist2d;

	// go through all meshes in thus regions
	for (int i = 0; i < (mesh_horcnt * mesh_vercnt); i++) {
		// each mesh has 4 knots
		for (int knotidx = 0; knotidx < 4; knotidx++) {
			// display window center locate @ (0,0), mesh info shife back to center O.
			// for each region, rollback center is (0,0)
			double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor
				- FISHEYE_REGION[rgn_idx].OutX - FISHEYE_REGION[rgn_idx].OutW / 2;
			double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor
				- FISHEYE_REGION[rgn_idx].OutY - FISHEYE_REGION[rgn_idx].OutH / 2;


			x = minmax(x - CenterXOffset, -view_w / 2 + 1, view_w / 2 - 1);
			y = minmax(y - CenterYOffset, -view_h / 2 + 1, view_h / 2 - 1);


			x = x / Aspect_gainX;
			y = y / Aspect_gainY;

			if (bAspect == true) {
				x = x * (1 - 0.333 * (100 - XYRatio)/100);
				y = y * (1 - 0.333 * (100 - XYRatio)/100);
			} else {
				x = x * (1 - 0.333 * (100 - XRatio)/100);
				y = y * (1 - 0.333 * (100 - YRatio)/100);
			}

			double rd = sqrt(x * x + y * y);

			// Zooming In/Out Control by ZoomH & ZoomV
			// 1.0 stands for +- 50% of zoomH ratio. => set as
			dist2d.x = x * (1 + k * ((rd / norm) * (rd / norm)));
			dist2d.y = y * (1 + k * ((rd / norm) * (rd / norm)));

			// update source mesh-info here
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0 + CenterXOffset;
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0 + CenterYOffset;
		}
	}
}

void _RegionView2( FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, FISHEYE_MOUNT_MODE_E MOUNT, double x0, double y0, double r)
{
	if (FISHEYE_REGION[rgn_idx].ViewMode != PROJECTION_REGION)
		return;

	//int view_w = FISHEYE_REGION[rgn_idx].OutW;
	//int view_h = FISHEYE_REGION[rgn_idx].OutH;
	int tot_mesh_cnt = FISHEYE_REGION[rgn_idx].MeshHor * FISHEYE_REGION[rgn_idx].MeshVer;

	// rotation matrix to point out view center of this region.
	RotMatrix3D mat0;
	GenRotMatrix3D_YXZ(&mat0, &FISHEYE_REGION[rgn_idx]);

	Vector3D pix3d;
	Vector2D dist2d;
	int X_offset = FISHEYE_REGION[rgn_idx].OutX + FISHEYE_REGION[rgn_idx].OutW / 2;
	int Y_offset = FISHEYE_REGION[rgn_idx].OutY + FISHEYE_REGION[rgn_idx].OutH / 2;
	double w_ratio = 1.0 * (FISHEYE_REGION[rgn_idx].ZoomH - 2048) / 2048;
	double h_ratio = 1.0 * (FISHEYE_REGION[rgn_idx].ZoomV - 2048) / 2048;

	// go through all meshes in thus regions
	// mat0 is decided by view angle defined in re gion config.
	for (int i = 0; i < tot_mesh_cnt; i++)
	{
		// each mesh has 4 knots
		for (int knotidx = 0; knotidx < 4; knotidx++)
		{
			// display window center locate @ (0,0), mesh info shife back to center O.
			// for each region, rollback center is (0,0)
			double x = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].xcor - X_offset;
			double y = FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[i].knot[knotidx].ycor - Y_offset;
			double z = r;


			if (MOUNT == FISHEYE_DESKTOP_MOUNT) {
				x = -x;
				y = -y;
			}


			// Zooming In/Out Control by ZoomH & ZoomV
			// 1.0 stands for +- 50% of zoomH ratio. => set as
			x = x *(1 + w_ratio);
			y = y *(1 + h_ratio);

			//printf("(x,y)=(%f,%f)\n\r", x, y);

			pix3d.x = x;
			pix3d.y = y;
			pix3d.z = z;

			// Region Porjection Model
			Rotate3D(&pix3d, &mat0);
			Equidistant(&pix3d, &dist2d, r);

			// update source mesh-info here
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor = dist2d.x + x0;
			FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor = dist2d.y + y0;
			//printf("rgn(%d) mesh(%d) node(%d) (%lf %lf)\n", rgn_idx, i, knotidx, FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].xcor, FISHEYE_REGION[rgn_idx].SrcRgnMeshInfo[i].knot[knotidx].ycor);
		}
	}
}

void _get_region_src_mesh_list(FISHEYE_MOUNT_MODE_E MOUNT, FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx, double x0, double y0, double r)
{
	// ViewModeType to decide mapping & UI contrl parameters.
	PROJECTION_MODE ViewModeType = FISHEYE_REGION[rgn_idx].ViewMode;

	if (ViewModeType == PROJECTION_REGION)
		_RegionView2( FISHEYE_REGION, rgn_idx, MOUNT, x0, y0, r);
	else if (ViewModeType == PROJECTION_PANORAMA_360)
		_Panorama360View2( FISHEYE_REGION, rgn_idx, MOUNT, x0, y0, r);
	else if (ViewModeType == PROJECTION_PANORAMA_180)
		_Panorama180View2(FISHEYE_REGION, rgn_idx, MOUNT, x0, y0, r);
	else if (ViewModeType == PROJECTION_LDC)
		_LDC_View(FISHEYE_REGION, rgn_idx, x0, y0);
	else
		printf("ERROR!!! THIS CASE SHOULDNOTHAPPEN!!!!!!\n\r");
}

void _get_region_dst_mesh_list(FISHEYE_REGION_ATTR* FISHEYE_REGION, int rgn_idx)
{
	if (FISHEYE_REGION[rgn_idx].RegionValid != 1)
		return;

	// Destination Mesh-Info Allocation
	int view_w = FISHEYE_REGION[rgn_idx].OutW;
	int view_h = FISHEYE_REGION[rgn_idx].OutH;
	int mesh_horcnt = FISHEYE_REGION[rgn_idx].MeshHor;
	int mesh_vercnt = FISHEYE_REGION[rgn_idx].MeshVer;

	//printf("view_w(%d) view_h(%d) mesh_horcnt(%d) mesh_vercnt(%d)\n", view_w, view_h, mesh_horcnt, mesh_vercnt);
	int knot_horcnt = mesh_horcnt + 1;
	int knot_vercnt = mesh_vercnt + 1;
	// tmp internal buffer
	// maximum knot number = 1024 (pre-set)
	COORDINATE2D *meshknot_hit_buf = malloc(sizeof(COORDINATE2D) * (knot_horcnt * knot_vercnt + 1));

	// 1st loop: to find mesh infos. on source ( backward projection )
	// hit index for buffer
	int knotcnt = 0;
	int mesh_w = (view_w / mesh_horcnt);
	int mesh_h = (view_h / mesh_vercnt);
	int half_w = view_w / 2;
	int half_h = view_h / 2;
	//printf("mesh_w(%d) mesh_h(%d)\n", mesh_w, mesh_h);
#if 0
	for (int y = -half_h; y < half_h; ++y) {
		bool yknot_hit = ((y + half_h) % mesh_h) == 0;
		bool LastMeshFixY = ((y + half_h) == (mesh_h * mesh_vercnt)) && (view_h != mesh_h * mesh_vercnt);

		for (int x = -half_w; x < half_w; x++) {
			bool xknot_hit = ((x + half_w) % mesh_w) == 0;
			bool hitknot = ((xknot_hit && yknot_hit)
					|| (((x + 1) == (half_w)) && yknot_hit)
					|| (xknot_hit && ((y + 1) == (half_h)))
					|| (((x + 1) == (half_w)) && ((y + 1) == (half_h))));

			// LastMeshFix is to fix unequal mesh block counts.
			bool LastMeshFixX = ((x + half_w) == (mesh_w * mesh_horcnt)) && (view_w != mesh_w * mesh_horcnt);

			hitknot = hitknot && !(LastMeshFixX || LastMeshFixY);

			if (hitknot) {
				meshknot_hit_buf[knotcnt].xcor = FISHEYE_REGION[rgn_idx].OutX + (x + half_w);
				meshknot_hit_buf[knotcnt].ycor = FISHEYE_REGION[rgn_idx].OutY + (y + half_h);
				//printf("%d(%lf %lf)\n", knotcnt, meshknot_hit_buf[knotcnt].xcor, meshknot_hit_buf[knotcnt].ycor);
				knotcnt += 1;
			}
		}
	}
#else
	int y = -half_h;
	for (int j = knot_horcnt; j > 0; --j) {
		int x = -half_w;
		for (int i = knot_horcnt; i > 0; --i) {
			meshknot_hit_buf[knotcnt].xcor = FISHEYE_REGION[rgn_idx].OutX + (x + half_w);
			meshknot_hit_buf[knotcnt].ycor = FISHEYE_REGION[rgn_idx].OutY + (y + half_h);
			//printf("%d(%lf %lf)\n", knotcnt, meshknot_hit_buf[knotcnt].xcor, meshknot_hit_buf[knotcnt].ycor);
			knotcnt += 1;
			x += mesh_w;
			if (i == 2)
				x = half_w;
		}
		y += mesh_h;
		if (j == 2)
			y = half_h;
	}
#endif

	meshknot_hit_buf[knotcnt].xcor = 0xFFFFFFFF;	//End of Knot List.
	meshknot_hit_buf[knotcnt].ycor = 0xFFFFFFFF;	//End of Knot List
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "knotcnt(%d)\n", knotcnt);
	// tmp for debug

	for (int j = 0; j < mesh_vercnt; j++) {
		for (int i = 0; i < mesh_horcnt; i++) {
			int meshidx = j * mesh_horcnt + i;
			int knotidx = j * (mesh_horcnt + 1) + i;	// knot num = mesh num +1 ( @ horizon )

			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].xcor = meshknot_hit_buf[knotidx].xcor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].ycor = meshknot_hit_buf[knotidx].ycor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].xcor = meshknot_hit_buf[knotidx + 1].xcor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].ycor = meshknot_hit_buf[knotidx + 1].ycor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].xcor = meshknot_hit_buf[knotidx + (mesh_horcnt + 1)].xcor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].ycor = meshknot_hit_buf[knotidx + (mesh_horcnt + 1)].ycor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].xcor = meshknot_hit_buf[knotidx + 1 + (mesh_horcnt + 1)].xcor;
			FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].ycor = meshknot_hit_buf[knotidx + 1 + (mesh_horcnt + 1)].ycor;

			//printf("mesh[%d] = (%f,%f),(%f,%f),(%f,%f),(%f,%f)\n\r", meshidx, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].xcor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[0].ycor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].xcor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[1].ycor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].xcor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[2].ycor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].xcor, FISHEYE_REGION[rgn_idx].DstRgnMeshInfo[meshidx].knot[3].ycor);
			//system("pause");
		}
	}

	free(meshknot_hit_buf);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
RECT_S aspect_ratio_resize(SIZE_S in, SIZE_S out)
{
	RECT_S rect;
	float ratio = MIN2((float)out.u32Width / in.u32Width, (float)out.u32Height / in.u32Height);

	rect.u32Height = (float)in.u32Height * ratio + 0.5;
	rect.u32Width = (float)in.u32Width * ratio + 0.5;
	rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	rect.s32Y = (out.u32Height - rect.u32Height) >> 1;
	return rect;
}

void _LOAD_REGION_CONFIG(FISHEYE_ATTR* FISHEYE_CONFIG, FISHEYE_REGION_ATTR* FISHEYE_REGION)
{
	// to make sure parameters aligned to frame ratio
	double width_sec = FISHEYE_CONFIG->OutW_disp / 40;
	double height_sec = FISHEYE_CONFIG->OutH_disp / 40;

	// load default settings
	if (FISHEYE_CONFIG->UsageMode == MODE_02_1O4R )
	{
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 8;
		FISHEYE_REGION[0].MeshHor = 8;
		FISHEYE_REGION[0].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[0].ThetaX = M_PI / 4;
		FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ThetaY = 0;
		FISHEYE_REGION[0].ZoomH = 2048;					// zooming factor for horizontal:	0 ~ 4095 to control view
		FISHEYE_REGION[0].ZoomV = 2048;					// zooming factor for vertical:		0 ~ 4095 to control view
		FISHEYE_REGION[0].Pan  = UI_CTRL_VALUE_CENTER;			// center = 180, value = 0 ~ 360	=> value = +- 180 degrees
		FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;			// center = 180, value = 0 ~ 360	=> value = +-30 degrees
		FISHEYE_REGION[0].OutW = (width_sec * 15);
		FISHEYE_REGION[0].OutH = (height_sec * 20);
		FISHEYE_REGION[0].OutX = 0;
		FISHEYE_REGION[0].OutY = 0;

		FISHEYE_REGION[1].RegionValid = 1;
		FISHEYE_REGION[1].MeshVer = 8;
		FISHEYE_REGION[1].MeshHor = 8;
		FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[1].ThetaX = M_PI / 4;
		FISHEYE_REGION[1].ThetaZ = M_PI / 2;
		FISHEYE_REGION[1].ThetaY = 0;
		FISHEYE_REGION[1].ZoomH = 2048;
		FISHEYE_REGION[1].ZoomV = 2048;
		FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[1].OutW = (width_sec * 15);
		FISHEYE_REGION[1].OutH = (height_sec * 20);
		FISHEYE_REGION[1].OutX = (width_sec * 15);
		FISHEYE_REGION[1].OutY = (height_sec * 0);

		FISHEYE_REGION[2].RegionValid = 1;
		FISHEYE_REGION[2].MeshVer = 8;
		FISHEYE_REGION[2].MeshHor = 8;
		FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[2].ThetaX = M_PI / 4;
		FISHEYE_REGION[2].ThetaZ = M_PI;
		FISHEYE_REGION[2].ThetaY = 0;
		FISHEYE_REGION[2].ZoomH = 2048;
		FISHEYE_REGION[2].ZoomV = 2048;
		FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[2].OutW = (width_sec * 15);
		FISHEYE_REGION[2].OutH = (height_sec * 20);
		FISHEYE_REGION[2].OutX = (width_sec * 0);
		FISHEYE_REGION[2].OutY = (height_sec * 20);

		FISHEYE_REGION[3].RegionValid = 1;
		FISHEYE_REGION[3].MeshVer = 8;
		FISHEYE_REGION[3].MeshHor = 8;
		FISHEYE_REGION[3].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[3].ThetaX = M_PI / 4;
		FISHEYE_REGION[3].ThetaZ = 3 * M_PI / 2;
		FISHEYE_REGION[3].ThetaY = 0;
		FISHEYE_REGION[3].ZoomH = 2048;
		FISHEYE_REGION[3].ZoomV = 2048;
		FISHEYE_REGION[3].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[3].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[3].OutW = (width_sec * 15);
		FISHEYE_REGION[3].OutH = (height_sec * 20);
		FISHEYE_REGION[3].OutX = (width_sec * 15);
		FISHEYE_REGION[3].OutY = (height_sec * 20);

		FISHEYE_REGION[4].RegionValid = 0;
	}
	else if (FISHEYE_CONFIG->UsageMode == MODE_03_4R)
	{
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 16;
		FISHEYE_REGION[0].MeshHor = 16;
		FISHEYE_REGION[0].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[0].ThetaX = 0.4*M_PI;
		FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ThetaY = 0;
		FISHEYE_REGION[0].ZoomH = 2048;					// zooming factor for horizontal:	0 ~ 4095 to control view
		FISHEYE_REGION[0].ZoomV = 2048;					// zooming factor for vertical:		0 ~ 4095 to control view
		FISHEYE_REGION[0].Pan = UI_CTRL_VALUE_CENTER;			// center = 180, value = 0 ~ 360	=> value = +- 180 degrees
		FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;			// center = 180, value = 0 ~ 360	=> value = +-30 degrees
		FISHEYE_REGION[0].OutW = (width_sec * 20);
		FISHEYE_REGION[0].OutH = (height_sec * 20);
		FISHEYE_REGION[0].OutX = 0;//(width_sec * 2);
		FISHEYE_REGION[0].OutY = 0;//(height_sec * 2);

		FISHEYE_REGION[1].RegionValid = 1;
		FISHEYE_REGION[1].MeshVer = 16;
		FISHEYE_REGION[1].MeshHor = 16;
		FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[1].ThetaX = 0.4 * M_PI;
		FISHEYE_REGION[1].ThetaZ = M_PI / 2;
		FISHEYE_REGION[1].ThetaY = 0;
		FISHEYE_REGION[1].ZoomH = 2048;
		FISHEYE_REGION[1].ZoomV = 2048;
		FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[1].OutW = (width_sec * 20);
		FISHEYE_REGION[1].OutH = (height_sec * 20);
		FISHEYE_REGION[1].OutX = (width_sec * 20);
		FISHEYE_REGION[1].OutY = (height_sec * 0);

		FISHEYE_REGION[2].RegionValid = 1;
		FISHEYE_REGION[2].MeshVer = 16;
		FISHEYE_REGION[2].MeshHor = 16;
		FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[2].ThetaX = 0.4 * M_PI;
		FISHEYE_REGION[2].ThetaZ = M_PI;
		FISHEYE_REGION[2].ThetaY = 0;
		FISHEYE_REGION[2].ZoomH = 2048;
		FISHEYE_REGION[2].ZoomV = 2048;
		FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[2].OutW = (width_sec * 20);
		FISHEYE_REGION[2].OutH = (height_sec * 20);
		FISHEYE_REGION[2].OutX = (width_sec * 0);
		FISHEYE_REGION[2].OutY = (height_sec * 20);

		FISHEYE_REGION[3].RegionValid = 1;
		FISHEYE_REGION[3].MeshVer = 16;
		FISHEYE_REGION[3].MeshHor = 16;
		FISHEYE_REGION[3].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[3].ThetaX = 0.4 * M_PI;
		FISHEYE_REGION[3].ThetaZ = 3 * M_PI / 2;
		FISHEYE_REGION[3].ThetaY = 0;
		FISHEYE_REGION[3].ZoomH = 2048;
		FISHEYE_REGION[3].ZoomV = 2048;
		FISHEYE_REGION[3].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[3].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[3].OutW = (width_sec * 20);
		FISHEYE_REGION[3].OutH = (height_sec * 20);
		FISHEYE_REGION[3].OutX = (width_sec * 20);
		FISHEYE_REGION[3].OutY = (height_sec * 20);

		FISHEYE_REGION[4].RegionValid = 0;
	}
	else if (FISHEYE_CONFIG->UsageMode == MODE_04_1P2R)
	{
		// Region #1 => Panorama 180
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 16;
		FISHEYE_REGION[0].MeshHor = 16;
		FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
		//FISHEYE_REGION[0].ThetaX = 0;
		//FISHEYE_REGION[0].ThetaY = 0;
		//FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ZoomH = 4096;					// value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
		FISHEYE_REGION[0].ZoomV = 1920;					// value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
		FISHEYE_REGION[0].Pan   = UI_CTRL_VALUE_CENTER;			// value range = 0 ~ 360, => -180 ~ 0 ~ +180
		FISHEYE_REGION[0].Tilt  = UI_CTRL_VALUE_CENTER;			// value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
		FISHEYE_REGION[0].OutW  = (width_sec * 40);
		FISHEYE_REGION[0].OutH  = (height_sec * 22);
		FISHEYE_REGION[0].OutX  = 0;					//(width_sec * 1);
		FISHEYE_REGION[0].OutY  = 0;					//height_sec * 1);
		//FISHEYE_REGION[0].InRadius = 50;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].OutRadius = 450;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].PanEnd = 180;

		FISHEYE_REGION[1].RegionValid = 1;
		FISHEYE_REGION[1].MeshVer = 8;
		FISHEYE_REGION[1].MeshHor = 8;
		FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[1].ThetaX = M_PI / 4;
		FISHEYE_REGION[1].ThetaZ = M_PI / 2;
		FISHEYE_REGION[1].ThetaY = 0;
		FISHEYE_REGION[1].ZoomH = 2048;
		FISHEYE_REGION[1].ZoomV = 2048;
		FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[1].OutW = (width_sec * 20);
		FISHEYE_REGION[1].OutH = (height_sec * 18);
		FISHEYE_REGION[1].OutX = (width_sec * 0);
		FISHEYE_REGION[1].OutY = (height_sec * 22);

		FISHEYE_REGION[2].RegionValid = 1;
		FISHEYE_REGION[2].MeshVer = 8;
		FISHEYE_REGION[2].MeshHor = 8;
		FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[2].ThetaX = M_PI / 4;
		FISHEYE_REGION[2].ThetaZ = M_PI;
		FISHEYE_REGION[2].ThetaY = 0;
		FISHEYE_REGION[2].ZoomH = 2048;
		FISHEYE_REGION[2].ZoomV = 2048;
		FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[2].OutW = (width_sec * 20);
		FISHEYE_REGION[2].OutH = (height_sec * 18);
		FISHEYE_REGION[2].OutX = (width_sec * 20);
		FISHEYE_REGION[2].OutY = (height_sec * 22);

		FISHEYE_REGION[3].RegionValid = 0;
		FISHEYE_REGION[4].RegionValid = 0;
	}
	else if (FISHEYE_CONFIG->UsageMode == MODE_05_1P2R)
	{
		// Region #1 => Panorama 180
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 16;
		FISHEYE_REGION[0].MeshHor = 16;
		FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
		//FISHEYE_REGION[0].ThetaX = 0;
		//FISHEYE_REGION[0].ThetaY = 0;
		//FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ZoomH = 3000;					// value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
		FISHEYE_REGION[0].ZoomV = 2048;					// value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
		FISHEYE_REGION[0].Pan  = UI_CTRL_VALUE_CENTER;			// value range = 0 ~ 360, => -180 ~ 0 ~ +180
		FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;			// value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
		FISHEYE_REGION[0].OutW = (width_sec * 27 );
		FISHEYE_REGION[0].OutH = (height_sec * 40);
		FISHEYE_REGION[0].OutX = 0;					//(width_sec * 1);
		FISHEYE_REGION[0].OutY = 0;					//height_sec * 1);
		//FISHEYE_REGION[0].InRadius = 50;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].OutRadius = 450;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].PanEnd = 180;

		FISHEYE_REGION[1].RegionValid = 1;
		FISHEYE_REGION[1].MeshVer = 8;
		FISHEYE_REGION[1].MeshHor = 8;
		FISHEYE_REGION[1].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[1].ThetaX = M_PI / 4;
		FISHEYE_REGION[1].ThetaZ = M_PI / 2;
		FISHEYE_REGION[1].ThetaY = 0;
		FISHEYE_REGION[1].ZoomH = 2048;
		FISHEYE_REGION[1].ZoomV = 2048;
		FISHEYE_REGION[1].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[1].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[1].OutW = (width_sec * 13);
		FISHEYE_REGION[1].OutH = (height_sec * 20);
		FISHEYE_REGION[1].OutX = (width_sec * 27);
		FISHEYE_REGION[1].OutY = (height_sec * 0);

		FISHEYE_REGION[2].RegionValid = 1;
		FISHEYE_REGION[2].MeshVer = 8;
		FISHEYE_REGION[2].MeshHor = 8;
		FISHEYE_REGION[2].ViewMode = PROJECTION_REGION;
		FISHEYE_REGION[2].ThetaX = M_PI / 4;
		FISHEYE_REGION[2].ThetaZ = M_PI;
		FISHEYE_REGION[2].ThetaY = 0;
		FISHEYE_REGION[2].ZoomH = 2048;
		FISHEYE_REGION[2].ZoomV = 2048;
		FISHEYE_REGION[2].Pan = UI_CTRL_VALUE_CENTER;			// theta-X
		FISHEYE_REGION[2].Tilt = UI_CTRL_VALUE_CENTER;			// theta-Z
		FISHEYE_REGION[2].OutW = (width_sec * 13);
		FISHEYE_REGION[2].OutH = (height_sec * 20);
		FISHEYE_REGION[2].OutX = (width_sec * 27);
		FISHEYE_REGION[2].OutY = (height_sec * 20);

		FISHEYE_REGION[3].RegionValid = 0;
		FISHEYE_REGION[4].RegionValid = 0;


	}
	else if (FISHEYE_CONFIG->UsageMode == MODE_06_1P)
	{
		// Region #1 => Panorama 180
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 30;
		FISHEYE_REGION[0].MeshHor = 30;
		FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
		//FISHEYE_REGION[0].ThetaX = 0;
		//FISHEYE_REGION[0].ThetaY = 0;
		//FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ZoomH = 4096;					// value = 0 ~ 4095, symmeterically control horizontal View Range, ex:  value = 4095 => hor view angle = -90 ~ + 90
		FISHEYE_REGION[0].ZoomV = 2800;					// value = 0 ~ 4095, symmetrically control vertical view range. ex: value = 4096, ver view angle = -90 ~ + 90
		FISHEYE_REGION[0].Pan  = UI_CTRL_VALUE_CENTER;			// value range = 0 ~ 360, => -180 ~ 0 ~ +180
		FISHEYE_REGION[0].Tilt = UI_CTRL_VALUE_CENTER;			// value = 0 ~ 360, center = 180 ( original ) => -180 ~ 0 ~ + 180
		FISHEYE_REGION[0].OutW = (width_sec * 40 );
		FISHEYE_REGION[0].OutH = (height_sec * 40);
		FISHEYE_REGION[0].OutX = 0;					//(width_sec * 1);
		FISHEYE_REGION[0].OutY = 0;					//height_sec * 1);
		//FISHEYE_REGION[0].InRadius = 50;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].OutRadius = 450;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].PanEnd = 180;

		FISHEYE_REGION[1].RegionValid = 0;
		FISHEYE_REGION[2].RegionValid = 0;
		FISHEYE_REGION[3].RegionValid = 0;
		FISHEYE_REGION[4].RegionValid = 0;
	} else if (FISHEYE_CONFIG->UsageMode == MODE_07_2P ) {
		//_Panorama360View2;
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 16;
		FISHEYE_REGION[0].MeshHor = 16;
		FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_360;
		FISHEYE_REGION[0].ThetaX = M_PI / 4;
		FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ThetaY = 0;
		//FISHEYE_REGION[0].ZoomH = 4095;				// Not Used in Panorama 360 Mode.
		FISHEYE_REGION[0].ZoomV = 4095;					// To ZoomIn OutRadius
		FISHEYE_REGION[0].Pan = 0;					// for panorama 360 => Pan is the label start position angle ( in degrees
		FISHEYE_REGION[0].Tilt = 300;					// to add shift offset vertical angle.
		FISHEYE_REGION[0].OutW = (width_sec * 40);
		FISHEYE_REGION[0].OutH = (height_sec * 20);
		FISHEYE_REGION[0].OutX = (width_sec * 0);
		FISHEYE_REGION[0].OutY = (height_sec * 0);
		FISHEYE_REGION[0].InRadius = 300;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		FISHEYE_REGION[0].OutRadius = 500;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		FISHEYE_REGION[0].PanEnd = 180;

		FISHEYE_REGION[1].RegionValid = 1;
		FISHEYE_REGION[1].MeshVer = 16;
		FISHEYE_REGION[1].MeshHor = 16;
		FISHEYE_REGION[1].ViewMode = PROJECTION_PANORAMA_360;
		FISHEYE_REGION[1].ThetaX = M_PI / 4;
		FISHEYE_REGION[1].ThetaZ = 0;
		FISHEYE_REGION[1].ThetaY = 0;
		//FISHEYE_REGION[1].ZoomH = 4095;				// Not Used in Panorama 360 Mode.
		FISHEYE_REGION[1].ZoomV = 4095;					// To ZoomIn OutRadius
		FISHEYE_REGION[1].Pan  = 240;					// for panorama 360 => Pan is the label start position angle ( in degrees
		FISHEYE_REGION[1].Tilt = 200;					// to add shift offset vertical angle.
		FISHEYE_REGION[1].OutW = (width_sec * 40);
		FISHEYE_REGION[1].OutH = (height_sec * 20);
		FISHEYE_REGION[1].OutX = (width_sec * 0);
		FISHEYE_REGION[1].OutY = (height_sec * 20);
		FISHEYE_REGION[1].InRadius = 0;					// a ratio to represent OutRadius length. 1 = full origina redius.  value/512 is the value.
		FISHEYE_REGION[1].OutRadius = 512;				// a ratio to represent OutRadius length. 1 = full origina redius.	value/512 is the value.
		FISHEYE_REGION[1].PanEnd = 60;					//

		FISHEYE_REGION[2].RegionValid = 0;
		FISHEYE_REGION[3].RegionValid = 0;
		FISHEYE_REGION[4].RegionValid = 0;
	} else if (FISHEYE_CONFIG->UsageMode == MODE_PANORAMA_180) {
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 16;
		FISHEYE_REGION[0].MeshHor = 16;
		FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_180;
		//FISHEYE_REGION[0].ThetaX = M_PI / 4;
		//FISHEYE_REGION[0].ThetaZ = 0;
		//FISHEYE_REGION[0].ThetaY = 0;
		FISHEYE_REGION[0].ZoomH = 4096;					// Not Used in Panorama 360 Mode.
		FISHEYE_REGION[0].ZoomV = 4096;					// To ZoomIn OutRadius
		FISHEYE_REGION[0].Pan = 180;					// for panorama 360 => Pan is the label start position angle ( in degrees
		FISHEYE_REGION[0].Tilt = 180;					// to add shift offset vertical angle.
		FISHEYE_REGION[0].OutW = (width_sec * 40);
		FISHEYE_REGION[0].OutH = (height_sec * 40);
		FISHEYE_REGION[0].OutX = 0;					//(width_sec * 1);
		FISHEYE_REGION[0].OutY = 0;					//height_sec * 1);
		//FISHEYE_REGION[0].InRadius = 50;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].OutRadius = 450;				// a ratio to represent OutRadius length. 1 => full origina redius.	value/512 is the value.
		//FISHEYE_REGION[0].PanEnd = 180;

		FISHEYE_REGION[1].RegionValid = 0;
		FISHEYE_REGION[2].RegionValid = 0;
		FISHEYE_REGION[3].RegionValid = 0;
		FISHEYE_REGION[4].RegionValid = 0;
	} else if (FISHEYE_CONFIG->UsageMode == MODE_PANORAMA_360) {
		SIZE_S in, out;
		RECT_S rect;

		in.u32Width = 2 * M_PI * FISHEYE_CONFIG->InRadius;
		in.u32Height = FISHEYE_CONFIG->InRadius;
		out.u32Width = (width_sec * 40);
		out.u32Height = (height_sec * 40);
		rect = aspect_ratio_resize(in, out);

		//_Panorama360View2;
		FISHEYE_REGION[0].RegionValid = 1;
		FISHEYE_REGION[0].MeshVer = 64;
		FISHEYE_REGION[0].MeshHor = 64;
		FISHEYE_REGION[0].ViewMode = PROJECTION_PANORAMA_360;
		FISHEYE_REGION[0].ThetaX = M_PI / 4;
		FISHEYE_REGION[0].ThetaZ = 0;
		FISHEYE_REGION[0].ThetaY = 0;
		//FISHEYE_REGION[0].ZoomH = 4095;				// Not Used in Panorama 360 Mode.
		FISHEYE_REGION[0].ZoomV = 4095;					// To ZoomIn OutRadius
		FISHEYE_REGION[0].Pan = 0;					// for panorama 360 => Pan is the label start position angle ( in degrees
		FISHEYE_REGION[0].Tilt = 180;					// to add shift offset vertical angle.
		FISHEYE_REGION[0].OutW = rect.u32Width;
		FISHEYE_REGION[0].OutH = rect.u32Height;
		FISHEYE_REGION[0].OutX = rect.s32X;				//(width_sec * 1);
		FISHEYE_REGION[0].OutY = rect.s32Y;				//(height_sec * 1);
		FISHEYE_REGION[0].InRadius = 0;					// a ratio to represent OutRadius length. 1 => full origina redius.	value = 0 ~ 4095,
		FISHEYE_REGION[0].OutRadius = 4095;				// a ratio to represent OutRadius length. 1 => full origina redius.	value = 0 ~ 4095
		FISHEYE_REGION[0].PanEnd = 359;

		FISHEYE_REGION[1].RegionValid = 0;
		FISHEYE_REGION[2].RegionValid = 0;
		FISHEYE_REGION[3].RegionValid = 0;
		FISHEYE_REGION[4].RegionValid = 0;
	} else {
		printf("Not done yet. !!!");
		//system("pause");
	}
}

void _LOAD_FRAME_CONFIG(FISHEYE_ATTR* FISHEYE_CONFIG)
{
	switch (FISHEYE_CONFIG->UsageMode)
	{
	case MODE_PANORAMA_360:
		FISHEYE_CONFIG->RgnNum = 1;
		break;
	case MODE_PANORAMA_180:
		FISHEYE_CONFIG->RgnNum = 1;
		break;
	case MODE_01_1O:
		FISHEYE_CONFIG->RgnNum = 1;
		break;
	case MODE_02_1O4R:
		FISHEYE_CONFIG->RgnNum = 4;	//	1O should be handled in scaler block.
		break;
	case MODE_03_4R:
		FISHEYE_CONFIG->RgnNum = 4;
		break;
	case MODE_04_1P2R:
		FISHEYE_CONFIG->RgnNum = 3;
		break;
	case MODE_05_1P2R:
		FISHEYE_CONFIG->RgnNum = 3;
		break;
	case MODE_06_1P:
		FISHEYE_CONFIG->RgnNum = 1;
		break;
	case MODE_07_2P:
		FISHEYE_CONFIG->RgnNum = 2;//
		break;
	default:
		printf("UsageMode Error!!!");
		//system("pause");
		break;
	}
}

/**
 *  generate_mesh_on_fisheye: generate mesh for fisheye
 *
 * @param param: mesh parameters.
 * @param faces: the nodes describe the faces
 */
static int generate_mesh_on_fisheye(FISHEYE_ATTR* FISHEYE_CONFIG, FISHEYE_REGION_ATTR* FISHEYE_REGION
	, int X_TILE_NUMBER, int Y_TILE_NUMBER
	, uint16_t *reorder_mesh_id_list, int **reorder_mesh_tbl, uint64_t mesh_tbl_phy_addr)
{
	int mesh_tbl_num;	// get number of meshes
	double x0, y0, r;	// infos of src_img, (x0,y0) = center of image,  r = radius of image.

	struct timespec start, end;

	clock_gettime(CLOCK_MONOTONIC, &start);

	x0 = FISHEYE_CONFIG->InCenterX;
	y0 = FISHEYE_CONFIG->InCenterY;
	r = FISHEYE_CONFIG->InRadius;
	// In Each Mode, for Every Region:
	for (int rgn_idx = 0; rgn_idx < FISHEYE_CONFIG->RgnNum; rgn_idx++) {
		// check region valid first
		if (!FISHEYE_REGION[rgn_idx].RegionValid)
			return -1;

		// get & store region mesh info.
		_get_region_dst_mesh_list(FISHEYE_REGION, rgn_idx);

		// Get Source Mesh-Info Projected from Destination by Differet ViewModw.
		_get_region_src_mesh_list(FISHEYE_CONFIG->MntMode, FISHEYE_REGION, rgn_idx, x0, y0, r);
	}
	//combine all region meshs - mesh projection done.
	_get_frame_mesh_list(FISHEYE_CONFIG, FISHEYE_REGION);

	mesh_tbl_num	= FISHEYE_CONFIG->TotalMeshNum;
	float src_x_mesh_tbl[mesh_tbl_num][4];
	float src_y_mesh_tbl[mesh_tbl_num][4];
	float dst_x_mesh_tbl[mesh_tbl_num][4];
	float dst_y_mesh_tbl[mesh_tbl_num][4];
	for (int mesh_idx = 0; mesh_idx < FISHEYE_CONFIG->TotalMeshNum; mesh_idx++) {
		for (int knotidx = 0; knotidx < 4; knotidx++) {
			src_x_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->SrcRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
			src_y_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->SrcRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
			dst_x_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->DstRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
			dst_y_mesh_tbl[mesh_idx][knotidx] = FISHEYE_CONFIG->DstRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
		}
	}

	int dst_height, dst_width;
	if (FISHEYE_CONFIG->rotate_index == 1 || FISHEYE_CONFIG->rotate_index == 3) {
		dst_height	= FISHEYE_CONFIG->OutW_disp;
		dst_width	= FISHEYE_CONFIG->OutH_disp;
	} else {
		dst_height	= FISHEYE_CONFIG->OutH_disp;
		dst_width	= FISHEYE_CONFIG->OutW_disp;
	}

	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh_tbl_num = %d\n", mesh_tbl_num);

	/////////////////////////////////////////////////////////////////////////////////////
	// mesh scan order preprocessing
	/////////////////////////////////////////////////////////////////////////////////////
	int Y_SUBTILE_NUMBER = ceil((float)dst_height / (float)NUMBER_Y_LINE_A_SUBTILE);
	int MAX_MESH_NUM_A_TILE = ((64 * 2) / X_TILE_NUMBER) * (1 + ceil((4*128)/(float)dst_height)); // (maximum horizontal meshes number x 2)/horizontal tiles number
	uint16_t mesh_scan_tile_mesh_id_list[MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER];

	int mesh_id_list_entry_num =
		mesh_scan_preproc_3(dst_width, dst_height,
				    dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num, mesh_scan_tile_mesh_id_list,
				    X_TILE_NUMBER, NUMBER_Y_LINE_A_SUBTILE,
				    Y_TILE_NUMBER, MAX_MESH_NUM_A_TILE);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "mesh_id_list_entry_num = %d\n", mesh_id_list_entry_num);

	int isrc_x_mesh_tbl[mesh_tbl_num][4];
	int isrc_y_mesh_tbl[mesh_tbl_num][4];
	int idst_x_mesh_tbl[mesh_tbl_num][4];
	int idst_y_mesh_tbl[mesh_tbl_num][4];

	mesh_coordinate_float2fixed(src_x_mesh_tbl, src_y_mesh_tbl, dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num,
				    isrc_x_mesh_tbl, isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl);

	int reorder_mesh_tbl_entry_num, reorder_mesh_id_list_entry_num;

	mesh_tbl_reorder_and_parse_3(mesh_scan_tile_mesh_id_list, mesh_id_list_entry_num,
				     isrc_x_mesh_tbl, isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl,
				     X_TILE_NUMBER, Y_TILE_NUMBER, Y_SUBTILE_NUMBER, reorder_mesh_tbl,
				     &reorder_mesh_tbl_entry_num, reorder_mesh_id_list,
				     &reorder_mesh_id_list_entry_num, mesh_tbl_phy_addr);

	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));
	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh id list size (bytes) = %d\n", (reorder_mesh_id_list_entry_num * 2));

	clock_gettime(CLOCK_MONOTONIC, &end);
	CVI_TRACE_GDC(CVI_DBG_INFO, "time consumed: %fms\n",
		      (CVI_FLOAT)get_diff_in_us(start, end) / 1000);
	return 0;
}

void _fisheye_attr_map(const FISHEYE_ATTR_S *pstFisheyeAttr, SIZE_S out_size, FISHEYE_ATTR *FISHEYE_CONFIG
	, FISHEYE_REGION_ATTR *FISHEYE_REGION)
{
	FISHEYE_CONFIG->MntMode = pstFisheyeAttr->enMountMode;
	FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
	FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
	FISHEYE_CONFIG->InCenterX = pstFisheyeAttr->s32HorOffset;
	FISHEYE_CONFIG->InCenterY = pstFisheyeAttr->s32VerOffset;
	// TODO: how to handl radius
	FISHEYE_CONFIG->InRadius = MIN2(FISHEYE_CONFIG->InCenterX, FISHEYE_CONFIG->InCenterY);
	FISHEYE_CONFIG->FStrength = pstFisheyeAttr->s32FanStrength;
	FISHEYE_CONFIG->TCoef  = pstFisheyeAttr->u32TrapezoidCoef;

	FISHEYE_CONFIG->RgnNum = pstFisheyeAttr->u32RegionNum;

	CVI_TRACE_GDC(CVI_DBG_INFO, "OutW_disp(%d) OutH_disp(%d)\n"
		, FISHEYE_CONFIG->OutW_disp, FISHEYE_CONFIG->OutH_disp);
	CVI_TRACE_GDC(CVI_DBG_INFO, "InCenterX(%d) InCenterY(%d)\n"
		, FISHEYE_CONFIG->InCenterX, FISHEYE_CONFIG->InCenterY);
	CVI_TRACE_GDC(CVI_DBG_INFO, "FStrength(%lf) TCoef(%lf) RgnNum(%d)\n"
		, FISHEYE_CONFIG->FStrength, FISHEYE_CONFIG->TCoef, FISHEYE_CONFIG->RgnNum);

	for (int i = 0; i < MAX_REGION_NUM; ++i)
		FISHEYE_REGION[i].RegionValid = 0;

	for (int i = 0; i < FISHEYE_CONFIG->RgnNum; ++i) {
		FISHEYE_REGION[i].RegionValid = 1;

		FISHEYE_REGION[i].ZoomH = pstFisheyeAttr->astFishEyeRegionAttr[i].u32HorZoom;
		FISHEYE_REGION[i].ZoomV = pstFisheyeAttr->astFishEyeRegionAttr[i].u32VerZoom;
		FISHEYE_REGION[i].Pan = pstFisheyeAttr->astFishEyeRegionAttr[i].u32Pan;
		FISHEYE_REGION[i].Tilt = pstFisheyeAttr->astFishEyeRegionAttr[i].u32Tilt;
		FISHEYE_REGION[i].OutW = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.u32Width;
		FISHEYE_REGION[i].OutH = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.u32Height;
		FISHEYE_REGION[i].OutX = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.s32X;
		FISHEYE_REGION[i].OutY = pstFisheyeAttr->astFishEyeRegionAttr[i].stOutRect.s32Y;
		FISHEYE_REGION[i].InRadius = pstFisheyeAttr->astFishEyeRegionAttr[i].u32InRadius;
		FISHEYE_REGION[i].OutRadius = pstFisheyeAttr->astFishEyeRegionAttr[i].u32OutRadius;
		if (pstFisheyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_NORMAL) {
			FISHEYE_REGION[i].MeshVer = 16;
			FISHEYE_REGION[i].MeshHor = 16;
			FISHEYE_REGION[i].ViewMode = PROJECTION_REGION;
			FISHEYE_REGION[i].ThetaX = 0.4*M_PI;
			FISHEYE_REGION[i].ThetaZ = 0;
			FISHEYE_REGION[i].ThetaY = 0;
		} else if (pstFisheyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA) {
			FISHEYE_REGION[i].MeshVer = 32;
			FISHEYE_REGION[i].MeshHor = 32;
			FISHEYE_REGION[i].ViewMode = PROJECTION_PANORAMA_180;
		} else if (pstFisheyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_360_PANORAMA) {
			FISHEYE_REGION[i].MeshVer = (FISHEYE_CONFIG->RgnNum == 1) ? 64 : 32;
			FISHEYE_REGION[i].MeshHor = (FISHEYE_CONFIG->RgnNum == 1) ? 64 : 32;
			FISHEYE_REGION[i].ViewMode = PROJECTION_PANORAMA_360;
			FISHEYE_REGION[i].ThetaX = M_PI / 4;
			FISHEYE_REGION[i].ThetaZ = 0;
			FISHEYE_REGION[i].ThetaY = 0;
			FISHEYE_REGION[i].PanEnd = FISHEYE_REGION[i].Pan
				+ 360 * pstFisheyeAttr->astFishEyeRegionAttr[i].u32HorZoom / 4096;
		}
		CVI_TRACE_GDC(CVI_DBG_INFO, "Region(%d) ViewMode(%d) MeshVer(%d) MeshHor(%d)\n"
			, i, FISHEYE_REGION[i].ViewMode, FISHEYE_REGION[i].MeshVer, FISHEYE_REGION[i].MeshHor);
		CVI_TRACE_GDC(CVI_DBG_INFO, "ZoomH(%lf) ZoomV(%lf) Pan(%d) Tilt(%d) PanEnd(%d)\n"
			, FISHEYE_REGION[i].ZoomH, FISHEYE_REGION[i].ZoomV
			, FISHEYE_REGION[i].Pan, FISHEYE_REGION[i].Tilt, FISHEYE_REGION[i].PanEnd);
		CVI_TRACE_GDC(CVI_DBG_INFO, "InRadius(%lf) OutRadius(%lf) Rect(%d %d %d %d)\n"
			, FISHEYE_REGION[i].InRadius, FISHEYE_REGION[i].OutRadius
			, FISHEYE_REGION[i].OutX, FISHEYE_REGION[i].OutY
			, FISHEYE_REGION[i].OutW, FISHEYE_REGION[i].OutH);
	}
}

void mesh_gen_fisheye(SIZE_S in_size, SIZE_S out_size, const FISHEYE_ATTR_S *pstFisheyeAttr
	, uint64_t mesh_phy_addr, void *mesh_vir_addr, ROTATION_E rot)
{
	FISHEYE_ATTR *FISHEYE_CONFIG;
	FISHEYE_REGION_ATTR *FISHEYE_REGION;

	FISHEYE_CONFIG = (FISHEYE_ATTR *)calloc(1, sizeof(*FISHEYE_CONFIG));
	if (!FISHEYE_CONFIG) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "memory insufficient for fisheye config\n");
		return;
	}
	FISHEYE_REGION = (FISHEYE_REGION_ATTR *)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
	if (!FISHEYE_REGION) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "memory insufficient for fisheye region config\n");
		return;
	}

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
		, in_size.u32Width, in_size.u32Height
		, out_size.u32Width, out_size.u32Height);

	if (pstFisheyeAttr->enUseMode > 0) {
		double x0, y0, r;	// infos of src_img, (x0,y0) = center of image,  r = radius of image.
		x0 = pstFisheyeAttr->s32HorOffset;
		y0 = pstFisheyeAttr->s32VerOffset;
		r = MIN2(x0, y0);

		FISHEYE_CONFIG->Enable	= true;
		FISHEYE_CONFIG->BgEnable = true;
		FISHEYE_CONFIG->MntMode = pstFisheyeAttr->enMountMode;
		FISHEYE_CONFIG->UsageMode = pstFisheyeAttr->enUseMode;
		FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
		FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
		FISHEYE_CONFIG->BgColor.R = 0;
		FISHEYE_CONFIG->BgColor.G = 0;
		FISHEYE_CONFIG->BgColor.B = 0;
		FISHEYE_CONFIG->InCenterX = x0;	// front-end set.
		FISHEYE_CONFIG->InCenterY = y0;	// front-end set.
		FISHEYE_CONFIG->InRadius = r;	// front-end set.
		FISHEYE_CONFIG->FStrength = pstFisheyeAttr->s32FanStrength;

		_LOAD_FRAME_CONFIG(FISHEYE_CONFIG);
		_LOAD_REGION_CONFIG(FISHEYE_CONFIG, FISHEYE_REGION);
	} else
		_fisheye_attr_map(pstFisheyeAttr, out_size, FISHEYE_CONFIG, FISHEYE_REGION);

	FISHEYE_CONFIG->rotate_index = rot;

	int X_TILE_NUMBER, Y_TILE_NUMBER;
	CVI_U32 mesh_id_size, mesh_tbl_size;
	CVI_U64 mesh_id_phy_addr, mesh_tbl_phy_addr;
	CVI_BOOL is_4K = (in_size.u32Width > 2048) && (out_size.u32Width > 2048);

	X_TILE_NUMBER = is_4K ? 4 : 1;
	Y_TILE_NUMBER = is_4K ? 4 : 1;

	// calculate mesh_id/mesh_tbl's size in bytes.
	mesh_tbl_size = 0x60000;
	mesh_id_size = 0x30000;

	// Decide the position of mesh in memory.
	mesh_id_phy_addr = mesh_phy_addr;
	mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
		     , mesh_id_phy_addr, mesh_tbl_phy_addr);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

	int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

	// Provide virtual address to write mesh.
	reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
	generate_mesh_on_fisheye(FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
		, (uint16_t *)mesh_vir_addr
		, reorder_mesh_tbl, mesh_tbl_phy_addr);
	free(FISHEYE_CONFIG);
	free(FISHEYE_REGION);
}

void _ldc_attr_map(const LDC_ATTR_S *pstLDCAttr, SIZE_S out_size, FISHEYE_ATTR *FISHEYE_CONFIG
	, FISHEYE_REGION_ATTR *FISHEYE_REGION)
{
	FISHEYE_CONFIG->MntMode = 0;
	FISHEYE_CONFIG->OutW_disp = out_size.u32Width;
	FISHEYE_CONFIG->OutH_disp = out_size.u32Height;
	FISHEYE_CONFIG->InCenterX = out_size.u32Width >> 1;
	FISHEYE_CONFIG->InCenterY = out_size.u32Height >> 1;
	// TODO: how to handl radius
	FISHEYE_CONFIG->InRadius = MIN2(FISHEYE_CONFIG->InCenterX, FISHEYE_CONFIG->InCenterY);
	FISHEYE_CONFIG->FStrength = 0;
	FISHEYE_CONFIG->TCoef  = 0;

	FISHEYE_CONFIG->RgnNum = 1;

	CVI_TRACE_GDC(CVI_DBG_INFO, "OutW_disp(%d) OutH_disp(%d)\n"
		, FISHEYE_CONFIG->OutW_disp, FISHEYE_CONFIG->OutH_disp);

	for (int i = 0; i < MAX_REGION_NUM; ++i)
		FISHEYE_REGION[i].RegionValid = 0;

	for (int i = 0; i < FISHEYE_CONFIG->RgnNum; ++i) {
		FISHEYE_REGION[i].RegionValid = 1;

		//FISHEYE_REGION[i].ZoomH = pstLDCAttr->bAspect;
		FISHEYE_REGION[i].ZoomV = pstLDCAttr->bAspect;
		FISHEYE_REGION[i].Pan = pstLDCAttr->s32XYRatio;
		FISHEYE_REGION[i].PanEnd = pstLDCAttr->s32DistortionRatio;
		FISHEYE_REGION[i].Tilt = 180;
		FISHEYE_REGION[i].OutW = FISHEYE_CONFIG->OutW_disp;
		FISHEYE_REGION[i].OutH = FISHEYE_CONFIG->OutH_disp;
		FISHEYE_REGION[i].OutX = 0;
		FISHEYE_REGION[i].OutY = 0;
		FISHEYE_REGION[i].InRadius = pstLDCAttr->s32CenterXOffset;
		FISHEYE_REGION[i].OutRadius = pstLDCAttr->s32CenterYOffset;
		FISHEYE_REGION[i].MeshVer = 16;
		FISHEYE_REGION[i].MeshHor = 16;
		FISHEYE_REGION[i].ViewMode = PROJECTION_LDC;
		FISHEYE_REGION[i].ThetaX = pstLDCAttr->s32XRatio;
		FISHEYE_REGION[i].ThetaZ = pstLDCAttr->s32YRatio;
		FISHEYE_REGION[i].ThetaY = 0;

		CVI_TRACE_GDC(CVI_DBG_INFO, "Region(%d) ViewMode(%d) MeshVer(%d) MeshHor(%d)\n"
			, i, FISHEYE_REGION[i].ViewMode, FISHEYE_REGION[i].MeshVer, FISHEYE_REGION[i].MeshHor);
		CVI_TRACE_GDC(CVI_DBG_INFO, "bAspect(%d) XYRatio(%d) DistortionRatio(%d)\n"
			, (bool)FISHEYE_REGION[i].ZoomV, FISHEYE_REGION[i].Pan, FISHEYE_REGION[i].PanEnd);
		CVI_TRACE_GDC(CVI_DBG_INFO, "XRatio(%d) XYRatio(%d)\n"
			, (int)FISHEYE_REGION[i].ThetaX, (int)FISHEYE_REGION[i].ThetaZ);
		CVI_TRACE_GDC(CVI_DBG_INFO, "CenterXOffset(%lf) CenterYOffset(%lf) Rect(%d %d %d %d)\n"
			, FISHEYE_REGION[i].InRadius, FISHEYE_REGION[i].OutRadius
			, FISHEYE_REGION[i].OutX, FISHEYE_REGION[i].OutY
			, FISHEYE_REGION[i].OutW, FISHEYE_REGION[i].OutH);
	}
}

int set_mesh_size(int mesh_hor, int mesh_ver)
{
	if (mesh_hor <= 0 || mesh_ver <= 0) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "bad param , mesh_hor:(%d), mesh_ver:(%d)\n", mesh_hor, mesh_ver);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (mesh_hor * mesh_ver > MESH_MAX_SIZE) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "mesh size is too large, max mesh size is (%d).\n", MESH_MAX_SIZE);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	meshHor = mesh_hor;
	meshVer = mesh_ver;

	return CVI_SUCCESS;
}

int get_mesh_size(int *p_mesh_hor, int *p_mesh_ver)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, p_mesh_hor);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, p_mesh_ver);
	*p_mesh_hor = meshHor;
	*p_mesh_ver = meshVer;

	return CVI_SUCCESS;
}

void mesh_gen_rotation(SIZE_S in_size, SIZE_S out_size, ROTATION_E rot
	, uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
	FISHEYE_ATTR *FISHEYE_CONFIG;
	LDC_ATTR_S stLDCAttr = { .bAspect = true, .s32XYRatio = 100,
		.s32CenterXOffset = 0, .s32CenterYOffset = 0, .s32DistortionRatio = 0 };
	FISHEYE_REGION_ATTR *FISHEYE_REGION;
	int nMeshhor, nMeshVer;

	FISHEYE_CONFIG = (FISHEYE_ATTR *)calloc(1, sizeof(*FISHEYE_CONFIG));
	if (!FISHEYE_CONFIG) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "memory insufficient for fisheye config\n");
		return;
	}
	FISHEYE_REGION = (FISHEYE_REGION_ATTR *)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
	if (!FISHEYE_REGION) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "memory insufficient for fisheye region config\n");
		return;
	}

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
		, in_size.u32Width, in_size.u32Height
		, out_size.u32Width, out_size.u32Height);

	_ldc_attr_map(&stLDCAttr, in_size, FISHEYE_CONFIG, FISHEYE_REGION);
	get_mesh_size(&nMeshhor, &nMeshVer);
	FISHEYE_REGION[0].MeshHor = nMeshhor;
	FISHEYE_REGION[0].MeshVer = nMeshVer;

	FISHEYE_CONFIG->rotate_index = rot;

	int X_TILE_NUMBER, Y_TILE_NUMBER;
	CVI_U32 mesh_id_size, mesh_tbl_size;
	CVI_U64 mesh_id_phy_addr, mesh_tbl_phy_addr;

	X_TILE_NUMBER = DIV_UP(in_size.u32Width, 122);
	Y_TILE_NUMBER = 8;

	// calculate mesh_id/mesh_tbl's size in bytes.
	mesh_tbl_size = 0x20000;
	mesh_id_size = 0x40000;

	// Decide the position of mesh in memory.
	mesh_id_phy_addr = mesh_phy_addr;
	mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
		     , mesh_id_phy_addr, mesh_tbl_phy_addr);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

	int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

	// Provide virtual address to write mesh.
	reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
	generate_mesh_on_fisheye(FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
		, (uint16_t *)mesh_vir_addr
		, reorder_mesh_tbl, mesh_tbl_phy_addr);
	free(FISHEYE_CONFIG);
	free(FISHEYE_REGION);
}

CVI_S32 mesh_gen_ldc(SIZE_S in_size, SIZE_S out_size, const LDC_ATTR_S *pstLDCAttr,
			uint64_t mesh_phy_addr, void *mesh_vir_addr, ROTATION_E rot)
{
	FISHEYE_ATTR *FISHEYE_CONFIG;
	FISHEYE_REGION_ATTR *FISHEYE_REGION;

	FISHEYE_CONFIG = (FISHEYE_ATTR *)calloc(1, sizeof(*FISHEYE_CONFIG));
	if (!FISHEYE_CONFIG) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "memory insufficient for fisheye config\n");
		return CVI_ERR_GDC_NOMEM;
	}
	FISHEYE_REGION = (FISHEYE_REGION_ATTR *)calloc(1, sizeof(*FISHEYE_REGION) * MAX_REGION_NUM);
	if (!FISHEYE_REGION) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "memory insufficient for fisheye region config\n");
		return CVI_ERR_GDC_NOMEM;
	}

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "in_size(%d %d) out_size(%d %d)\n"
		, in_size.u32Width, in_size.u32Height
		, out_size.u32Width, out_size.u32Height);

	_ldc_attr_map(pstLDCAttr, out_size, FISHEYE_CONFIG, FISHEYE_REGION);
	FISHEYE_CONFIG->rotate_index = rot;

	int X_TILE_NUMBER, Y_TILE_NUMBER;
	CVI_U32 mesh_id_size, mesh_tbl_size;
	CVI_U64 mesh_id_phy_addr, mesh_tbl_phy_addr;

	X_TILE_NUMBER = DIV_UP(in_size.u32Width, 122);
	Y_TILE_NUMBER = 8;

	// calculate mesh_id/mesh_tbl's size in bytes.
	mesh_tbl_size = 0x60000;
	mesh_id_size = 0x50000;

	// Decide the position of mesh in memory.
	mesh_id_phy_addr = mesh_phy_addr;
	mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "phy-addr of mesh id(%#"PRIx64") mesh_tbl(%#"PRIx64")\n"
		     , mesh_id_phy_addr, mesh_tbl_phy_addr);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

	int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

	// Provide virtual address to write mesh.
	reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);
	generate_mesh_on_fisheye(FISHEYE_CONFIG, FISHEYE_REGION, X_TILE_NUMBER, Y_TILE_NUMBER
		, (uint16_t *)mesh_vir_addr
		, reorder_mesh_tbl, mesh_tbl_phy_addr);
	free(FISHEYE_CONFIG);
	free(FISHEYE_REGION);

	return CVI_SUCCESS;
}

int CNV_MESH_EDGE[4][2] = { { 0, 1 }, { 1, 3 }, { 2, 0 }, { 3, 2 } };

int cnv_mesh_coordinate_float2fixed(float src_x_mesh_tbl[][4], float src_y_mesh_tbl[][4], float dst_x_mesh_tbl[][4],
				    float dst_y_mesh_tbl[][4], int mesh_tbl_num, int isrc_x_mesh_tbl[][4],
				    int isrc_y_mesh_tbl[][4], int idst_x_mesh_tbl[][4], int idst_y_mesh_tbl[][4])
{
	int64_t MAX_VAL;
	int64_t MIN_VAL;
	double tmp_val;
	int64_t val;

	MAX_VAL = 1;
	MAX_VAL <<= (DEWARP_COORD_MBITS + DEWARP_COORD_NBITS);
	MAX_VAL -= 1;
	MIN_VAL = -1 * MAX_VAL;

	for (int i = 0; i < mesh_tbl_num; i++) {
		for (int j = 0; j < 4; j++) {
			tmp_val = (src_x_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			isrc_x_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

			tmp_val = (src_y_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			isrc_y_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

			tmp_val = (dst_x_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			idst_x_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);

			tmp_val = (dst_y_mesh_tbl[i][j] * (double)(1 << DEWARP_COORD_NBITS));
			val = (tmp_val >= 0) ? (int64_t)(tmp_val + 0.5) : (int64_t)(tmp_val - 0.5);
			idst_y_mesh_tbl[i][j] = (int)CLIP(val, MIN_VAL, MAX_VAL);
		}
	}

	return 0;
}

int cnv_mesh_tbl_reorder_and_parse_3(uint16_t *mesh_scan_tile_mesh_id_list, int mesh_id_list_entry_num,
				     int isrc_x_mesh_tbl[][4], int isrc_y_mesh_tbl[][4], int idst_x_mesh_tbl[][4],
				     int idst_y_mesh_tbl[][4], int X_TILE_NUMBER, int Y_TILE_NUMBER,
				     int Y_SUBTILE_NUMBER, int **reorder_mesh_tbl, int *reorder_mesh_tbl_entry_num,
				     uint16_t *reorder_mesh_id_list, int *reorder_mesh_id_list_entry_num,
				     uint64_t mesh_tbl_phy_addr)
{
	int mesh = -1;
	int reorder_mesh;
	int mesh_id_idx = 0;
	int mesh_idx = 0;
	int *reorder_id_map = (int *)malloc(sizeof(int) * 128 * 128);

	int *reorder_mesh_slice_tbl = reorder_mesh_tbl[0]; // initial mesh_tbl to

	int i = 0;
	// int ext_mem_addr_alignment = 32;

	(void) mesh_id_list_entry_num;
	(void)Y_SUBTILE_NUMBER;

	// frame start ID
	reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];

	for (int k = 0; k < Y_TILE_NUMBER; k++) {
		for (int j = 0; j < X_TILE_NUMBER; j++) {
			reorder_mesh_tbl[j + k * X_TILE_NUMBER] = reorder_mesh_slice_tbl + mesh_idx;

			reorder_mesh = 0;
#if 1 // (JAMMY) clear to -1
			for (int l = 0; l < 128 * 128; l++) {
				reorder_id_map[l] = -1;
			}
#else
			memset(reorder_id_map, 0xff, sizeof(sizeof(int) * 128 * 128));
#endif

			// slice start ID
			mesh = mesh_scan_tile_mesh_id_list[i++];
			reorder_mesh_id_list[mesh_id_idx++] = mesh;

// (JAMMY) replace with phy-addr later.
// reorder mesh table address -> reorder mesh id list header
#if 0
			uintptr_t addr = (uintptr_t)reorder_mesh_tbl[j + k * X_TILE_NUMBER];

			reorder_mesh_id_list[mesh_id_idx++] = addr & 0x0000000fff;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0000fff000) >> 12;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0fff000000) >> 24;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0xf000000000) >> 36;
#else
			CVI_U64 addr = mesh_tbl_phy_addr + ((uintptr_t)reorder_mesh_tbl[j + k * X_TILE_NUMBER] -
							    (uintptr_t)reorder_mesh_tbl[0]);

			reorder_mesh_id_list[mesh_id_idx++] = addr & 0x0000000fff;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0000fff000) >> 12;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0x0fff000000) >> 24;
			reorder_mesh_id_list[mesh_id_idx++] = (addr & 0xf000000000) >> 36;
#endif

			// slice src and width -> reorder mesh id list header
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile x src
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile width
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile y src
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++]; // tile height

			while (mesh_scan_tile_mesh_id_list[i] != MESH_ID_FSE) {
				mesh = mesh_scan_tile_mesh_id_list[i++];
				if (mesh == MESH_ID_FST || mesh == MESH_ID_FSS) {
					continue;
				} else if (mesh == MESH_ID_FSP ||
					   /* mesh == MESH_ID_FED */ mesh == MESH_ID_FTE) {
					reorder_mesh_id_list[mesh_id_idx++] = mesh; // meta-data header
				} else /* if (mesh != MESH_ID_FED) */ {
					if (reorder_id_map[mesh] == -1) {
					reorder_id_map[mesh] = reorder_mesh;
					reorder_mesh++;

					for (int l = 0; l < 4; l++) {
						reorder_mesh_slice_tbl[mesh_idx++] = isrc_x_mesh_tbl[mesh][l];
						reorder_mesh_slice_tbl[mesh_idx++] = isrc_y_mesh_tbl[mesh][l];
						reorder_mesh_slice_tbl[mesh_idx++] = idst_x_mesh_tbl[mesh][l];
						reorder_mesh_slice_tbl[mesh_idx++] = idst_y_mesh_tbl[mesh][l];
					}
					}

					reorder_mesh_id_list[mesh_id_idx++] = reorder_id_map[mesh];
				}
			}

			// slice end ID
			reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];
		}
	}

	// frame end ID
	reorder_mesh_id_list[mesh_id_idx++] = mesh_scan_tile_mesh_id_list[i++];

	*reorder_mesh_tbl_entry_num = mesh_idx;
	*reorder_mesh_id_list_entry_num = mesh_id_idx;

	free(reorder_id_map);

	return 0;
}

int cnv_position_transfer_rect2dist(double x, double y, double *newx, double *newy, double *param, int mapping,
				    double scale, int width, int height, int owidth, int oheight)
{
	double coeff_0 = PI * param[0] / 180.0;
	double coeff_1 = param[1];
	double coeff_2 = param[2];
	double coeff_3 = param[3];
	double coeff_4 = param[4];

	double phi, radius2, radius;
	double x0, y0, x1, y1;
	double m11, m12, m13, m21, m22, m23;

	// normalize
	double PID2 = PI / 2;
	double norm_r = sqrt((width / 2.0) * (width / 2.0) + (height / 2.0) * (height / 2.0));
	double onorm_r = sqrt((owidth / 2.0) * (owidth / 2.0) + (oheight / 2.0) * (oheight / 2.0));

	x0 = x - width / 2;
	y0 = y - height / 2;
	phi = atan2(y0, x0);
	y0 = y0 / norm_r;
	y0 = y0 * scale;
	x0 = x0 / norm_r;
	x0 = x0 * scale;
	// polarize
	radius2 = y0 * y0 + x0 * x0;
	radius = sqrt(radius2);

	// 360 finsheye param
	double theta_src = param[0] * PI / 180.0;
	double theta_tot = param[1] * PI / 180.0;
	double radius_in = param[2];
	double radius_out = param[3];
	double lens_center_x = (double)owidth / 2;
	double lens_center_y = (double)oheight / 2;
	double theta = 0.0;

	switch (mapping) {
	case 1: // r <= r^2
		// distortion
		radius = radius * radius;
		// depolarize
		x1 = radius * cos(phi);
		y1 = radius * sin(phi);
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;
		break;
	case 2: // radial distortion
#if 0
		// distortion
		icoeff_1 = -1 * coeff_1;
		icoeff_2 = 0.f;
		icoeff_3 = 3 * coeff_1 * coeff_1 - coeff_3;
		icoeff_4 = 0.f;
		radius = radius * (1 + radius * (icoeff_1 + radius *
			(icoeff_2 + radius * (icoeff_3 + radius * icoeff_4))));
		// depolarize
		x1 = radius * cos(phi);
		y1 = radius * sin(phi);
		// denormalize
		*newx = (norm_r * x1) + width / 2;
		*newy = (norm_r * y1) + height / 2;
#endif
		*newx = x;
		*newy = y;
		break;
	case 3: // rotation
		// depolarize
		x1 = radius * cos(phi - coeff_0);
		y1 = radius * sin(phi - coeff_0);
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;
		break;
	case 4: // affine transform
		coeff_3 /= width;
		coeff_4 /= height;
		m11 = coeff_1 * cos(coeff_0);
		m12 = -1 * coeff_2 * sin(coeff_0);
		m13 = coeff_3 * coeff_1 * cos(coeff_0) - coeff_4 * coeff_2 * sin(coeff_0);
		m21 = coeff_1 * sin(coeff_0);
		m22 = coeff_2 * cos(coeff_0);
		m23 = coeff_3 * coeff_1 * sin(coeff_0) + coeff_4 * coeff_2 * cos(coeff_0);
		x1 = m11 * x0 + m12 * y0 + m13;
		y1 = m21 * x0 + m22 * y0 + m23;
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;

		break;
	case 5: // arcsin correction
		// distortion
		radius = sin(radius * PID2);
		// depolarize
		x1 = radius * cos(phi);
		y1 = radius * sin(phi);
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;
		break;
	case 6:
		x1 = x0 + y0 * tan(coeff_0);
		y1 = y0;
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;
		break;
	case 7:
		x1 = x0;
		y1 = y0 + x0 * tan(coeff_0);
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;
		break;
	case 8:
		x1 = x0;
		y1 = y0 + x0 * tan(coeff_0);
		// denormalize
		*newx = (onorm_r * x1) + owidth / 2;
		*newy = (onorm_r * y1) + oheight / 2;
		break;
	case 9:
		theta = -(x / (double)(width - 1)) * theta_tot;
		radius = (double)(height - 1 - y) / (double)(height - 1);
		radius = (radius * (1.0 - radius_in - radius_out) + radius_in) * (double)oheight / 2;
		*newx = radius * sin(theta - theta_src) + lens_center_x;
		*newy = radius * cos(theta - theta_src) + lens_center_y;
		break;
	default:
		*newx = x;
		*newy = y;
		break;
	}
	return 0;
}


// cnv

int cnv_mesh_scan_preproc_3(int width, int height, int dst_width, int dst_height, float src_x_mesh_tbl[][4],
			    float src_y_mesh_tbl[][4], float dst_x_mesh_tbl[][4], float dst_y_mesh_tbl[][4],
			    int mesh_tbl_num, uint16_t *mesh_scan_id_order, int Y_SUBTILE_NUMBER, int X_TILE_NUMBER,
			    int NUM_Y_LINE_A_SUBTILE, int NUM_X_LINE_A_TILE, int Y_TILE_NUMBER, int NUM_Y_LINE_A_TILE,
			    int MAX_MESH_NUM_A_TILE)
{
	int tile_idx_x, tile_idx_y;
	int current_dst_mesh_y_line_intersection_cnt, mesh_num_cnt = 0;

	int id_idx = 0;
	int *tmp_mesh_scan_id_order = (int *)malloc(sizeof(int) * (MAX_MESH_NUM_A_TILE * 16));

	(void) width;
	(void) height;
	(void) src_x_mesh_tbl;
	(void) src_y_mesh_tbl;
	(void) Y_SUBTILE_NUMBER;

	mesh_scan_id_order[id_idx] = MESH_ID_FST;
	id_idx++;

	for (tile_idx_y = 0; tile_idx_y < Y_TILE_NUMBER; tile_idx_y++) {
		int src_y = CLIP((tile_idx_y * NUM_Y_LINE_A_TILE), 0, dst_height);
		int dst_y = CLIP((src_y + NUM_Y_LINE_A_TILE), 0, dst_height);

		for (tile_idx_x = 0; tile_idx_x < X_TILE_NUMBER; tile_idx_x++) {
			mesh_scan_id_order[id_idx++] = MESH_ID_FSS;

			int src_x = CLIP((tile_idx_x * NUM_X_LINE_A_TILE), 0, dst_width);
			int dst_x = CLIP((src_x + NUM_X_LINE_A_TILE), 0, dst_width);

			mesh_scan_id_order[id_idx++] = src_x;
			mesh_scan_id_order[id_idx++] = dst_x - src_x;
			mesh_scan_id_order[id_idx++] = src_y;
			mesh_scan_id_order[id_idx++] = dst_y - src_y;

			for (int y = src_y; y < dst_y; y++) {
				// if first line of a tile, then initialization
				if (y % NUM_Y_LINE_A_SUBTILE == 0) {
					mesh_num_cnt = 0;
					for (int i = 0; i < MAX_MESH_NUM_A_TILE; i++)
						tmp_mesh_scan_id_order[i] = -1;

					// frame separator ID insertion
					mesh_scan_id_order[id_idx++] = MESH_ID_FSP;
				}

				for (int m = 0; m < mesh_tbl_num; m++) {
					current_dst_mesh_y_line_intersection_cnt = 0;
					int is_mesh_incorp_yline = 0;
					int min_x = MIN(MIN(dst_x_mesh_tbl[m][0], dst_x_mesh_tbl[m][1]),
							MIN(dst_x_mesh_tbl[m][2], dst_x_mesh_tbl[m][3]));
					int max_x = MAX(MAX(dst_x_mesh_tbl[m][0], dst_x_mesh_tbl[m][1]),
							MAX(dst_x_mesh_tbl[m][2], dst_x_mesh_tbl[m][3]));
					int min_y = MIN(MIN(dst_y_mesh_tbl[m][0], dst_y_mesh_tbl[m][1]),
							MIN(dst_y_mesh_tbl[m][2], dst_y_mesh_tbl[m][3]));
					int max_y = MAX(MAX(dst_y_mesh_tbl[m][0], dst_y_mesh_tbl[m][1]),
							MAX(dst_y_mesh_tbl[m][2], dst_y_mesh_tbl[m][3]));
					if (min_y <= y && y <= max_y && min_x <= src_x && dst_x <= max_x)
						is_mesh_incorp_yline = 1;

					if (!is_mesh_incorp_yline) {
					for (int k = 0; k < 4; k++) {
						float knot_dst_a_y = dst_y_mesh_tbl[m][CNV_MESH_EDGE[k][0]];
						float knot_dst_b_y = dst_y_mesh_tbl[m][CNV_MESH_EDGE[k][1]];
						float knot_dst_a_x = dst_x_mesh_tbl[m][CNV_MESH_EDGE[k][0]];
						float knot_dst_b_x = dst_x_mesh_tbl[m][CNV_MESH_EDGE[k][1]];
						float delta_a_y = (float)y - knot_dst_a_y;
						float delta_b_y = (float)y - knot_dst_b_y;
						int intersect_x = 0;

						if ((src_x <= knot_dst_a_x) && (knot_dst_a_x <= dst_x))
							intersect_x = 1;
						if ((src_x <= knot_dst_b_x) && (knot_dst_b_x <= dst_x))
							intersect_x = 1;

							// check whether if vertex connection
						if ((delta_a_y == 0.f) && (intersect_x == 1)) {
							current_dst_mesh_y_line_intersection_cnt += 2;
						}
							// check whether if edge connection
						else if ((delta_a_y * delta_b_y < 0) && (intersect_x == 1)) {
							current_dst_mesh_y_line_intersection_cnt += 2;
						}
							// otherwise no connection
					} // finish check in a mesh
					}

					if ((current_dst_mesh_y_line_intersection_cnt > 0) ||
					    (is_mesh_incorp_yline == 1)) {
						// check the mesh in list or not
						int isInList = 0;

					for (int i = 0; i < mesh_num_cnt; i++) {
						if (m == tmp_mesh_scan_id_order[i]) {
							isInList = 1;
							continue;
						}
					}
						// not in the list, then add the mesh to list
					if (!isInList) {
						tmp_mesh_scan_id_order[mesh_num_cnt] = m;
						mesh_num_cnt++;
					}
					}
				}

				// x direction reorder
				if (((y % NUM_Y_LINE_A_SUBTILE) == (NUM_Y_LINE_A_SUBTILE - 1)) ||
				    (y == dst_height - 1)) {
					for (int i = 0; i < mesh_num_cnt - 1; i++) {
					for (int j = 0; j < mesh_num_cnt - 1 - i; j++) {
						int m0 = tmp_mesh_scan_id_order[j];
						int m1 = tmp_mesh_scan_id_order[j + 1];
						float knot_m0_x0 = dst_x_mesh_tbl[m0][0];
						float knot_m0_x1 = dst_x_mesh_tbl[m0][1];
						float knot_m0_x2 = dst_x_mesh_tbl[m0][2];
						float knot_m0_x3 = dst_x_mesh_tbl[m0][3];
						float knot_m1_x0 = dst_x_mesh_tbl[m1][0];
						float knot_m1_x1 = dst_x_mesh_tbl[m1][1];
						float knot_m1_x2 = dst_x_mesh_tbl[m1][2];
						float knot_m1_x3 = dst_x_mesh_tbl[m1][3];

						int m0_min_x = MIN(MIN(knot_m0_x0, knot_m0_x1),
									MIN(knot_m0_x2, knot_m0_x3));
						int m1_min_x = MIN(MIN(knot_m1_x0, knot_m1_x1),
									MIN(knot_m1_x2, knot_m1_x3));

						if (m0_min_x > m1_min_x) {
							int tmp = tmp_mesh_scan_id_order[j];

							tmp_mesh_scan_id_order[j] =
								tmp_mesh_scan_id_order[j + 1];
							tmp_mesh_scan_id_order[j + 1] = tmp;
						}
					}
					}

					// mesh ID insertion
					for (int i = 0; i < mesh_num_cnt; i++) {
						mesh_scan_id_order[id_idx++] = tmp_mesh_scan_id_order[i];
					}

					// tile end ID insertion
					mesh_scan_id_order[id_idx++] = MESH_ID_FTE;
				}
			}

			// frame slice end ID insertion
			mesh_scan_id_order[id_idx++] = MESH_ID_FSE;
		}
	}

	// frame end ID insertion
	mesh_scan_id_order[id_idx++] = MESH_ID_FED;

	int mesh_id_list_entry_num = id_idx;

	free(tmp_mesh_scan_id_order);

	return mesh_id_list_entry_num;
}

/**
 * gen_mesh_f_xy_mesh_tbl: generate mesh from
 * src_x_mesh_tbl/src_y_mesh_tbl/dst_x_mesh_tbl/dst_y_mesh_tbl
 *
 * @param param: mesh parameters.
 */

static int gen_mesh_m_xy_mesh_tbl(float *mesh_data, int imgw, int imgh, uint16_t *reorder_mesh_id_list,
				  int **reorder_mesh_tbl, uint64_t mesh_tbl_phy_addr)
{
	int width, height, TotalMeshNum;
	int X_TILE_NUMBER, Y_TILE_NUMBER, tbl_w_mesh_num, tbl_h_mesh_num;
	// FILE *fpMesh;
	// if(mesh_data==NULL)
	//{
	//	fpMesh = fopen("init_mesh_file.txt", "r");
	//	fscanf(fpMesh, "%d %d %d %d %d %d %d", &TotalMeshNum, &X_TILE_NUMBER, &Y_TILE_NUMBER,
	//&tbl_w_mesh_num, &tbl_h_mesh_num, &width, &height); }else
	{
		TotalMeshNum = (int)mesh_data[0];
		X_TILE_NUMBER = (int)mesh_data[1];
		Y_TILE_NUMBER = (int)mesh_data[2];
		tbl_w_mesh_num = (int)mesh_data[3];
		tbl_h_mesh_num = (int)mesh_data[4];
		width = (int)mesh_data[5];
		height = (int)mesh_data[6];
	}
	if (imgw != width || imgh != height) {
		printf("TotalMeshNum, imgw, imgh, width, height: %d %d %d %d %d", TotalMeshNum, imgw, imgh, width,
		       height);
		return (-1);
	}

	int mesh_tbl_num = tbl_w_mesh_num * tbl_h_mesh_num; // get number of meshes
	float src_x_mesh_tbl[mesh_tbl_num][4];
	float src_y_mesh_tbl[mesh_tbl_num][4];
	float dst_x_mesh_tbl[mesh_tbl_num][4];
	float dst_y_mesh_tbl[mesh_tbl_num][4];

	float *ptrtmp = mesh_data + 7;
	int cc = 0;

	for (int mesh_idx = 0; mesh_idx < TotalMeshNum; mesh_idx++) {
		for (int knotidx = 0; knotidx < 4; knotidx++) {
			src_x_mesh_tbl[mesh_idx][knotidx] = ptrtmp[cc];
			cc++;
			src_y_mesh_tbl[mesh_idx][knotidx] = ptrtmp[cc];
			cc++;
			dst_x_mesh_tbl[mesh_idx][knotidx] = ptrtmp[cc];
			cc++;
			dst_y_mesh_tbl[mesh_idx][knotidx] = ptrtmp[cc];
			cc++;
		}
	}

	mesh_tbl_num = TotalMeshNum; // FISHEYE_CONFIG->TotalMeshNum;
	int dst_height = height; // FISHEYE_CONFIG->OutH_disp;
	int dst_width = width; // FISHEYE_CONFIG->OutW_disp;

	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh_tbl_num = %d\n", mesh_tbl_num);

	/////////////////////////////////////////////////////////////////////////////////////
	// mesh scan order preprocessing
	/////////////////////////////////////////////////////////////////////////////////////
	int Y_SUBTILE_NUMBER = ceil((float)dst_height / (float)NUMBER_Y_LINE_A_SUBTILE);
	int NUMBER_X_LINE_A_TILE = ceil((float)dst_width / (float)X_TILE_NUMBER / 2.0) * 2;
	int NUMBER_Y_LINE_A_TILE =
		(int)(ceil(ceil((float)dst_height / (float)Y_TILE_NUMBER) / (float)NUMBER_Y_LINE_A_SUBTILE)) *
		NUMBER_Y_LINE_A_SUBTILE;
	int MAX_MESH_NUM_A_TILE =
		((128 * 2) / X_TILE_NUMBER) *
		(1 +
		 ceil((4 * 128) / (float)dst_height)); // (maximum horizontal meshes number x 2)/horizontal tiles number
	printf("MAX_MESH_NUM_A_TILE: %d, %d, %d\n", MAX_MESH_NUM_A_TILE, Y_SUBTILE_NUMBER, X_TILE_NUMBER);
	uint16_t mesh_scan_tile_mesh_id_list[MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER];

	int mesh_id_list_entry_num =
		cnv_mesh_scan_preproc_3(width, height, dst_width, dst_height, src_x_mesh_tbl, src_y_mesh_tbl,
					dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num, mesh_scan_tile_mesh_id_list,
					Y_SUBTILE_NUMBER, X_TILE_NUMBER, NUMBER_Y_LINE_A_SUBTILE, NUMBER_X_LINE_A_TILE,
					Y_TILE_NUMBER, NUMBER_Y_LINE_A_TILE, MAX_MESH_NUM_A_TILE);

	printf("MAX_MESH_NUM_A_TILE: %d, %d, %d; mesh_id_list_entry_num: %d\n", MAX_MESH_NUM_A_TILE, Y_SUBTILE_NUMBER,
	       X_TILE_NUMBER, mesh_id_list_entry_num);

	int isrc_x_mesh_tbl[mesh_tbl_num][4];
	int isrc_y_mesh_tbl[mesh_tbl_num][4];
	int idst_x_mesh_tbl[mesh_tbl_num][4];
	int idst_y_mesh_tbl[mesh_tbl_num][4];

	cnv_mesh_coordinate_float2fixed(src_x_mesh_tbl, src_y_mesh_tbl, dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num,
					isrc_x_mesh_tbl, isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl);

	int reorder_mesh_tbl_entry_num, reorder_mesh_id_list_entry_num;

	cnv_mesh_tbl_reorder_and_parse_3(mesh_scan_tile_mesh_id_list, mesh_id_list_entry_num, isrc_x_mesh_tbl,
					 isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl, X_TILE_NUMBER,
					 Y_TILE_NUMBER, Y_SUBTILE_NUMBER, reorder_mesh_tbl, &reorder_mesh_tbl_entry_num,
					 reorder_mesh_id_list, &reorder_mesh_id_list_entry_num, mesh_tbl_phy_addr);

	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));
	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh id list size (bytes) = %d\n", (reorder_mesh_id_list_entry_num * 2));
	printf("mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));

	return 0;
}

#if 1
////int x_tile_nbr, int y_tile_nbr, int tbl_mesh_nbr_w, int tbl_mesh_nbr_h,
static int gen_mesh_f_xy_mesh_tbl(char *init_mesh_file, int imgw, int imgh, uint16_t *reorder_mesh_id_list,
				  int **reorder_mesh_tbl, uint64_t mesh_tbl_phy_addr)
{
	int width, height, TotalMeshNum;
	int X_TILE_NUMBER, Y_TILE_NUMBER, tbl_w_mesh_num, tbl_h_mesh_num;
	FILE *fpMesh;

	fpMesh = fopen(init_mesh_file, "r");
	fscanf(fpMesh, "%d %d %d %d %d %d %d", &TotalMeshNum, &X_TILE_NUMBER, &Y_TILE_NUMBER, &tbl_w_mesh_num,
	       &tbl_h_mesh_num, &width, &height);
	if (imgw != width || imgh != height) {
		printf("TotalMeshNum, imgw, imgh, width, height: %d %d %d %d %d", TotalMeshNum, imgw, imgh, width,
		       height);
		return (-1);
	}

	int mesh_tbl_num = tbl_w_mesh_num * tbl_h_mesh_num; // get number of meshes
	float src_x_mesh_tbl[mesh_tbl_num][4];
	float src_y_mesh_tbl[mesh_tbl_num][4];
	float dst_x_mesh_tbl[mesh_tbl_num][4];
	float dst_y_mesh_tbl[mesh_tbl_num][4];

	for (int mesh_idx = 0; mesh_idx < TotalMeshNum; mesh_idx++) {
		for (int knotidx = 0; knotidx < 4; knotidx++) {
			fscanf(fpMesh, "%f %f %f %f", &src_x_mesh_tbl[mesh_idx][knotidx],
			       &src_y_mesh_tbl[mesh_idx][knotidx], &dst_x_mesh_tbl[mesh_idx][knotidx],
			       &dst_y_mesh_tbl[mesh_idx][knotidx]);

			//			src_x_mesh_tbl[mesh_idx][knotidx] =
			//FISHEYE_CONFIG->SrcRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
			//			src_y_mesh_tbl[mesh_idx][knotidx] =
			//FISHEYE_CONFIG->SrcRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
			//			dst_x_mesh_tbl[mesh_idx][knotidx] =
			//FISHEYE_CONFIG->DstRgnMeshInfo[mesh_idx].knot[knotidx].xcor;
			//			dst_y_mesh_tbl[mesh_idx][knotidx] =
			//FISHEYE_CONFIG->DstRgnMeshInfo[mesh_idx].knot[knotidx].ycor;
		}
	}

	fclose(fpMesh);

	mesh_tbl_num = TotalMeshNum; // FISHEYE_CONFIG->TotalMeshNum;
	int dst_height = height; // FISHEYE_CONFIG->OutH_disp;
	int dst_width = width; // FISHEYE_CONFIG->OutW_disp;

	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh_tbl_num = %d\n", mesh_tbl_num);

	/////////////////////////////////////////////////////////////////////////////////////
	// mesh scan order preprocessing
	/////////////////////////////////////////////////////////////////////////////////////
	int Y_SUBTILE_NUMBER = ceil((float)dst_height / (float)NUMBER_Y_LINE_A_SUBTILE);
	int NUMBER_X_LINE_A_TILE = ceil((float)dst_width / (float)X_TILE_NUMBER / 2.0) * 2;
	int NUMBER_Y_LINE_A_TILE =
		(int)(ceil(ceil((float)dst_height / (float)Y_TILE_NUMBER) / (float)NUMBER_Y_LINE_A_SUBTILE)) *
		NUMBER_Y_LINE_A_SUBTILE;
	int MAX_MESH_NUM_A_TILE =
		((128 * 2) / X_TILE_NUMBER) *
		(1 +
		 ceil((4 * 128) / (float)dst_height)); // (maximum horizontal meshes number x 2)/horizontal tiles number
	uint16_t mesh_scan_tile_mesh_id_list[MAX_MESH_NUM_A_TILE * Y_SUBTILE_NUMBER * X_TILE_NUMBER];

	int mesh_id_list_entry_num =
		cnv_mesh_scan_preproc_3(width, height, dst_width, dst_height, src_x_mesh_tbl, src_y_mesh_tbl,
					dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num, mesh_scan_tile_mesh_id_list,
					Y_SUBTILE_NUMBER, X_TILE_NUMBER, NUMBER_Y_LINE_A_SUBTILE, NUMBER_X_LINE_A_TILE,
					Y_TILE_NUMBER, NUMBER_Y_LINE_A_TILE, MAX_MESH_NUM_A_TILE);

	int isrc_x_mesh_tbl[mesh_tbl_num][4];
	int isrc_y_mesh_tbl[mesh_tbl_num][4];
	int idst_x_mesh_tbl[mesh_tbl_num][4];
	int idst_y_mesh_tbl[mesh_tbl_num][4];

	cnv_mesh_coordinate_float2fixed(src_x_mesh_tbl, src_y_mesh_tbl, dst_x_mesh_tbl, dst_y_mesh_tbl, mesh_tbl_num,
					isrc_x_mesh_tbl, isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl);

	int reorder_mesh_tbl_entry_num, reorder_mesh_id_list_entry_num;

	cnv_mesh_tbl_reorder_and_parse_3(mesh_scan_tile_mesh_id_list, mesh_id_list_entry_num, isrc_x_mesh_tbl,
					 isrc_y_mesh_tbl, idst_x_mesh_tbl, idst_y_mesh_tbl, X_TILE_NUMBER,
					 Y_TILE_NUMBER, Y_SUBTILE_NUMBER, reorder_mesh_tbl, &reorder_mesh_tbl_entry_num,
					 reorder_mesh_id_list, &reorder_mesh_id_list_entry_num, mesh_tbl_phy_addr);

	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));
	CVI_TRACE_GDC(CVI_DBG_INFO, "mesh id list size (bytes) = %d\n", (reorder_mesh_id_list_entry_num * 2));

	printf("mesh table size (bytes) = %d\n", (reorder_mesh_tbl_entry_num * 4));
	printf("mesh id list size (bytes) = %d\n", (reorder_mesh_id_list_entry_num * 2));

	return 0;
}
#endif

void mesh_gen_cnv(const float *pfmesh_data, SIZE_S in_size, SIZE_S out_size, const FISHEYE_ATTR_S *pstFisheyeAttr,
		  uint64_t mesh_phy_addr, void *mesh_vir_addr)
{
	// FISHEYE_ATTR FISHEYE_CONFIG;
	// FISHEYE_REGION_ATTR FISHEYE_REGION[MAX_REGION_NUM];

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "in_size(%d %d) out_size(%d %d)\n", in_size.u32Width, in_size.u32Height,
		      out_size.u32Width, out_size.u32Height);

	int TotalMeshNum, tw, th;
	int tbl_w_mesh_num, tbl_h_mesh_num;
	int X_TILE_NUMBER, Y_TILE_NUMBER;
	CVI_U32 mesh_id_size, mesh_tbl_size;
	CVI_U64 mesh_id_phy_addr, mesh_tbl_phy_addr;

	(void) pstFisheyeAttr;

	// tbl_w_mesh_num = 64;
	// tbl_h_mesh_num = 64;
	X_TILE_NUMBER = 1;
	Y_TILE_NUMBER = 1;
	if (pfmesh_data == NULL) {
		FILE *fpMesh = fopen("init_mesh_file.txt", "r");

		fscanf(fpMesh, "%d %d %d %d %d %d %d", &TotalMeshNum, &X_TILE_NUMBER, &Y_TILE_NUMBER, &tbl_w_mesh_num,
		       &tbl_h_mesh_num, &tw, &th);
		fclose(fpMesh);
	} else {
		// TotalMeshNum = (int) pfmesh_data[0];
		X_TILE_NUMBER = (int)pfmesh_data[1];
		Y_TILE_NUMBER = (int)pfmesh_data[2];
	}

	// calculate mesh_id/mesh_tbl's size in bytes.
	mesh_tbl_size = 0x40000;
	mesh_id_size = 0x8000;

	// Decide the position of mesh in memory.
	mesh_id_phy_addr = mesh_phy_addr;
	mesh_tbl_phy_addr = ALIGN(mesh_phy_addr + mesh_id_size, 0x1000);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "phy-addr of mesh id(%#" PRIx64 ") mesh_tbl(%#" PRIx64 ")\n", mesh_id_phy_addr,
		      mesh_tbl_phy_addr);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "mesh_id_size(%d) mesh_tbl_size(%d)\n", mesh_id_size, mesh_tbl_size);

	int *reorder_mesh_tbl[X_TILE_NUMBER * Y_TILE_NUMBER];

	// Provide virtual address to write mesh.
	reorder_mesh_tbl[0] = mesh_vir_addr + (mesh_tbl_phy_addr - mesh_id_phy_addr);

	if (pfmesh_data == NULL) {
		gen_mesh_f_xy_mesh_tbl("init_mesh_file.txt", in_size.u32Width, in_size.u32Height,
				       (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr);
	} else {
		gen_mesh_m_xy_mesh_tbl((float *)pfmesh_data, in_size.u32Width, in_size.u32Height,
				       (uint16_t *)mesh_vir_addr, reorder_mesh_tbl, mesh_tbl_phy_addr);
	}
}

void mesh_gen_get_size(SIZE_S in_size, SIZE_S out_size, CVI_U32 *mesh_id_size, CVI_U32 *mesh_tbl_size)
{
	if (!mesh_id_size || !mesh_tbl_size)
		return;

	(void)in_size;
	(void)out_size;

	// CVI_GDC_MESH_SIZE_FISHEYE
	*mesh_tbl_size = 0x60000;
	*mesh_id_size = 0x50000;
}
