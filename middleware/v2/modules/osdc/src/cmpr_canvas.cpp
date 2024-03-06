#include "cmpr_canvas.h"

static bool is_in_range(int x, int begin, int end)
{
	return (x >= begin && x < end);
}

template <typename T> T clip(const T &n, const T &lower, const T &upper)
{
	return std::max(lower, std::min(n, upper));
}

int count_repeat_pixel(uint8_t *src, int pixel_sz, int pixel_num)
{
	int num = pixel_num - 1;
	for (int byte_i = 0; byte_i < pixel_sz; byte_i++) {
		uint8_t ref = src[byte_i];
		uint8_t *cur_ptr = &src[byte_i + pixel_sz];
		for (int cnt = 0; cnt < num; cnt++) {
			if (ref != (*cur_ptr)) {
				num = cnt;
				break;
			}
			cur_ptr += pixel_sz;
		}
	}

	return num + 1;
}

int recycle_obj_slices(vector<OBJ_SLICE> &slice_vec, int end_idx, int x)
{
	auto it = slice_vec.begin();
	int recycle_cnt = 0;
	for (int chk_cnt = 0; chk_cnt <= end_idx; chk_cnt++) {
		if (x >= (*it).x1) {
			it = slice_vec.erase(it);
			recycle_cnt++;
		} else {
			++it;
		}
	}

	return recycle_cnt;
}

void recycle_draw_obj(vector<DRAW_OBJ> &obj_vec, int y)
{
	for (auto it = obj_vec.begin(); it != obj_vec.end();) {
		if (y > (*it)._max_y) {
			it = obj_vec.erase(it);
		} else {
			++it;
		}
	}
}

int obj_project_on_line(vector<DRAW_OBJ> &obj_vec, vector<OBJ_SLICE> &slice_vec,
			int y, int &next_y, int canvas_width)
{
	for (int obj_i = 0; obj_i < (int)obj_vec.size(); obj_i++) {
		DRAW_OBJ &obj_attr = obj_vec[obj_i];
		if (is_in_range(y, obj_attr._min_y, obj_attr._max_y) == false) {
			next_y = min(next_y, max(obj_attr._min_y, y + 1));
			continue;
		}
		if (obj_attr.type == RECT) {
			OBJ_SLICE slice;
			slice.x0 = obj_attr.rect.x;
			slice.x1 = obj_attr.rect.x + obj_attr.rect.width;
			slice.obj_id = obj_i;
			slice_vec.push_back(slice);
			next_y = min(next_y, obj_attr._max_y);
		} else if (obj_attr.type == BIT_MAP ||
			   obj_attr.type == CMPR_BIT_MAP) {
			OBJ_SLICE slice;
			slice.x0 = obj_attr.bitmap.rect.x;
			slice.x1 = obj_attr.bitmap.rect.x +
				   obj_attr.bitmap.rect.width;
			slice.obj_id = obj_i;
			next_y = min(next_y, obj_attr._max_y);
			slice_vec.push_back(slice);
		} else if (obj_attr.type == STROKE_RECT) {
			int y1 = obj_attr.rect.y + obj_attr.rect.thickness;
			int y2 = obj_attr.rect.y + obj_attr.rect.height -
				 obj_attr.rect.thickness;
			if (is_in_range(y, y1, y2)) {
				OBJ_SLICE slice0, slice1;
				slice0.x0 = obj_attr.rect.x;
				slice0.x1 = obj_attr.rect.x +
					    obj_attr.rect.thickness;
				slice1.x1 =
					obj_attr.rect.x + obj_attr.rect.width;
				slice1.x0 = slice1.x1 - obj_attr.rect.thickness;
				slice0.obj_id = obj_i;
				slice1.obj_id = obj_i;
				slice_vec.push_back(slice0);
				slice_vec.push_back(slice1);
				next_y = min(next_y, y2);
			} else {
				OBJ_SLICE slice;
				slice.x0 = obj_attr.rect.x;
				slice.x1 =
					obj_attr.rect.x + obj_attr.rect.width;
				slice.obj_id = obj_i;
				slice_vec.push_back(slice);
				next_y = min(
					next_y,
					(y > y1) ?
						obj_attr.rect.y +
							obj_attr.rect.height :
						y1);
			}
		} else if (obj_attr.type == LINE) {
			float pt_x0, pt_x1;
			if (is_in_range(y, round(obj_attr.line._by[0]),
					round(obj_attr.line._by[1]))) {
				float delta_x0 = obj_attr.line._mx *
						 (y - obj_attr.line._by[0]);
				pt_x0 = obj_attr.line._bx[0] + delta_x0;
				float width = (y - obj_attr.line._by[0]) *
					      (obj_attr.line.ts_h /
					       (obj_attr.line._by[1] -
						obj_attr.line._by[0]));
				pt_x1 = pt_x0 - width;
			} else if (is_in_range(
					   y, round(obj_attr.line._by[1]),
					   round(obj_attr.line._ey[0] + 1.))) {
				float delta_x0 = obj_attr.line._mx *
						 (y - obj_attr.line._by[0]);
				pt_x0 = obj_attr.line._bx[0] + delta_x0;
				pt_x1 = pt_x0 - obj_attr.line.ts_h;
			} else {
				float delta_x0 = obj_attr.line._mx *
						 (y - obj_attr.line._by[1]);
				pt_x0 = obj_attr.line._bx[1] + delta_x0;
				float width = (obj_attr.line._ey[1] - y) *
					      (obj_attr.line.ts_h /
					       (obj_attr.line._ey[1] -
						obj_attr.line._ey[0]));
				pt_x1 = pt_x0 + width;
			}
			OBJ_SLICE slice;
			slice.x0 = clip((int)round(min(pt_x0, pt_x1)), 0, canvas_width-1);
			slice.x1 = clip((int)round(max(pt_x0, pt_x1)), 0, canvas_width-1);
			slice.obj_id = obj_i;
			slice_vec.push_back(slice);
			next_y = y + 1;
		}
	}

	return slice_vec.size();
}

void draw_cmpr_init(Cmpr_Canvas_Ctrl &ctrl, uint8_t *obuf, int buf_size,
		    Canvas_Attr *canvas)
{
	bool trial_enc = (obuf == nullptr) || (buf_size == 0);
	OSDCmpr_Ctrl *osdCmpr_ctrl = &ctrl.osdCmpr_ctrl;
	//memset(osdCmpr_ctrl, 0, sizeof(OSDCmpr_Ctrl));
	*osdCmpr_ctrl = {};
	osdCmpr_ctrl->reg_image_width = canvas->width;
	osdCmpr_ctrl->reg_image_height = canvas->height;
	osdCmpr_ctrl->reg_palette_mode_en = OSDEC_PAL_BD > 0;
	osdCmpr_ctrl->reg_palette_idx_bd = OSDEC_PAL_BD;
	osdCmpr_ctrl->reg_run_len_bd = OSDEC_RL_BD;
	osdCmpr_ctrl->reg_osd_format = canvas->format;
	osdCmpr_ctrl->pel_sz = osd_cmpr_get_pixel_sz(canvas->format);
	osd_cmpr_frame_init(&ctrl.osdCmpr_ctrl);
	int hdr_sz;
	if (trial_enc) {
		buf_size = 0;
		hdr_sz = 0;
	} else {
		osd_cmpr_enc_header(obuf, osdCmpr_ctrl);
		hdr_sz = osd_cmpr_get_header_sz();
	}
	init_stream(&ctrl.bitstream, &obuf[hdr_sz], buf_size - hdr_sz,
		    trial_enc);
	ctrl.md = NUM_OF_MODE; // must be invalid type to bypass the first run
	ctrl.rl_cnt = -1;
}

void draw_cmpr_finish(Cmpr_Canvas_Ctrl &ctrl)
{
	int dummy = 0;
	RGBA dummy_c;
	osd_cmpr_enc_const_pixel(dummy_c, ctrl.last_color, ctrl.rl_cnt, ctrl.md,
				 ctrl.code, dummy, true, OSDEC_MAX_RL,
				 &ctrl.osdCmpr_ctrl, &ctrl.bitstream);
}

void draw_cmpr_pixel(uint8_t *color_code_ptr, int length, bool is_first_pel,
		     Cmpr_Canvas_Ctrl &ctrl)
{
	RGBA cur_c =
		get_color(color_code_ptr, ctrl.osdCmpr_ctrl.reg_osd_format);
	osd_cmpr_enc_const_pixel(cur_c, ctrl.last_color, ctrl.rl_cnt, ctrl.md,
				 ctrl.code, length, is_first_pel, OSDEC_MAX_RL,
				 &ctrl.osdCmpr_ctrl, &ctrl.bitstream);
	while (length > 0) {
		osd_cmpr_enc_followed_run(cur_c, ctrl.rl_cnt, ctrl.md,
					  ctrl.code, length, OSDEC_MAX_RL,
					  &ctrl.osdCmpr_ctrl, &ctrl.bitstream);
	}
}

void draw_cmpr_canvas_line(Cmpr_Canvas_Ctrl &ctrl, vector<DRAW_OBJ> &obj_vec,
			   int y, int pixel_sz, vector<SEGMENT> &segment_vec)
{
	for (int seg_i = 0; seg_i < (int)segment_vec.size(); seg_i++) {
		SEGMENT &segment = segment_vec[seg_i];
		if (segment.is_const) {
			if(segment.width>0) {
				draw_cmpr_pixel((uint8_t *)&segment.color.code,
					segment.width, y == 0 && seg_i == 0,
					ctrl);
			}
		} else {
			int rep_cnt;
			uint8_t *src_ptr = segment.color.buf;
			if (segment.is_cmpr) {
				int pel_cnt = 0;
				while (pel_cnt < segment.width) {
					rep_cnt = min(src_ptr[0] + 1,
						      segment.width - pel_cnt);
					uint8_t *cur_ptr = &src_ptr[1];
					draw_cmpr_pixel(cur_ptr, rep_cnt,
							y == 0 && seg_i == 0 &&
								pel_cnt == 0,
							ctrl);
					src_ptr += (1 + pixel_sz);
					pel_cnt += rep_cnt;
				}
				segment.color.buf += *(segment.bs_len);
				obj_vec[segment.id].bitmap.bs_offset +=
					*(segment.bs_len);
				segment.bs_len++;
			} else {
				for (int pel_i = 0; pel_i < segment.width;
				     pel_i += rep_cnt) {
					uint8_t *cur_ptr =
						&src_ptr[pel_i * pixel_sz];
					rep_cnt = count_repeat_pixel(
						cur_ptr, pixel_sz,
						segment.width - pel_i);
					draw_cmpr_pixel(cur_ptr, rep_cnt,
							y == 0 && seg_i == 0 &&
								pel_i == 0,
							ctrl);
				}
				segment.color.buf +=
					(segment.stride * pixel_sz);
			}
		}
	}
}

void plot_segments_on_line(vector<DRAW_OBJ> &obj_vec, int width, int y,
			   int pixel_sz, int &next_y,
			   vector<SEGMENT> &segment_vec, uint32_t bg_color_code)
{
	vector<OBJ_SLICE> slice_vec;
	if (obj_project_on_line(obj_vec, slice_vec, y, next_y, width) == 0) {
		SEGMENT bg_seg{ true, false, (uint16_t)width, 0, bg_color_code, 0 };
		segment_vec.push_back(bg_seg);
		return;
	}
	int step;
	int next_slice_idx = -1;
	for (int x = 0; x < width; x += step) {
		int obj_idx = -1; // "-1": background
		int x0 = x;
		int x1 = width;
		int slices_num = slice_vec.size();
		if (next_slice_idx >= 0) {
			x0 = slice_vec[next_slice_idx].x0;
			x1 = slice_vec[next_slice_idx].x1;
			obj_idx = next_slice_idx;
		} else {
			// iter all slices
			for (int idx = (slices_num - 1); idx >= 0; idx--) {
				if (is_in_range(x, slice_vec[idx].x0,
						slice_vec[idx].x1)) {
					x0 = slice_vec[idx].x0;
					x1 = slice_vec[idx].x1;
					obj_idx = idx;
					break;
				}
			}
		}
		// find end point from higher priority slices
		next_slice_idx = -1;
		int end_x = x1;
		for (int hp_slice_i = (slices_num - 1); hp_slice_i > obj_idx;
		     hp_slice_i--) {
			if (is_in_range(slice_vec[hp_slice_i].x0, x0, end_x)) {
				end_x = slice_vec[hp_slice_i].x0;
				next_slice_idx = hp_slice_i;
			}
		}
		step = end_x - x;

		SEGMENT segment{ true, false, (uint16_t)width, 0, bg_color_code, 0 };
		segment.width = step;
		if (obj_idx >= 0) {
			DRAW_OBJ &obj = obj_vec[slice_vec[obj_idx].obj_id];
			segment.is_const = (obj.type == BIT_MAP ||
					    obj.type == CMPR_BIT_MAP) ?
						   false :
						   true;
			if (segment.is_const) {
				segment.color.code = obj.color.code;
			} else {
				segment.is_cmpr =
					(obj.type == BIT_MAP) ? false : true;
				int incr_y = y - obj.bitmap.rect.y;
				if (!segment.is_cmpr) {
					segment.stride = obj.bitmap.stride;
					segment.color.buf =
						obj.color.buf +
						((incr_y * segment.stride) +
						 (x - slice_vec[obj_idx].x0)) *
							pixel_sz;
				} else {
					segment.color.buf =
						&obj.color.buf
							 [obj.bitmap.bs_offset];
					segment.bs_len = &(
						(uint16_t *)
							obj.color.buf)[incr_y];
					segment.id = slice_vec[obj_idx].obj_id;
				}
			}
			// slices required recycle only when current segment is not background
			int recycle_cnt = recycle_obj_slices(slice_vec, obj_idx,
							     x + step);
			// correct next slice index due to recycled slices
			next_slice_idx -= recycle_cnt;
		}
		segment_vec.push_back(segment);
	} // end of draw a part of a slice
}

// ---------------------- cmpr_canvas main API ----------------------
int draw_cmpr_canvas(Canvas_Attr *canvas, DRAW_OBJ *objs, uint32_t obj_num,
		     uint8_t *obuf, int buf_size, uint32_t* p_osize)
{
	vector<DRAW_OBJ> obj_vec(objs, objs+obj_num);
	Cmpr_Canvas_Ctrl ctrl;
	draw_cmpr_init(ctrl, obuf, buf_size, canvas);
	size_t pixel_sz = osd_cmpr_get_pixel_sz(canvas->format);
	int y_step;
	for (int y = 0; y < canvas->height; y += y_step) {
		int next_y = canvas->height;
		vector<SEGMENT> segment_vec;
		plot_segments_on_line(obj_vec, canvas->width, y, pixel_sz,
				      next_y, segment_vec, canvas->bg_color_code);
		y_step = next_y - y;
		for (int line_i = 0; line_i < y_step; line_i++) {
			draw_cmpr_canvas_line(ctrl, obj_vec, y + line_i,
					      pixel_sz, segment_vec);
		}
		recycle_draw_obj(obj_vec, y + y_step - 1);
	}
	draw_cmpr_finish(ctrl);
	uint32_t stream_sz = (ctrl.bitstream.bit_pos + 7) >> 3; // in byte
	//add 16 bytes per DE's suggestion
	*p_osize = (((stream_sz + osd_cmpr_get_header_sz() + 15) >> 4) + 1) << 4; // 16byte align
	return ctrl.bitstream.status;
}

#if (CMPR_CANVAS_DBG)
void fill_n_pixel(void *dest, int width, int code, int pixel_sz)
{
	if (pixel_sz == 4) {
		fill_n((uint32_t *)dest, width, code);
	} else if (pixel_sz == 2) {
		fill_n((uint16_t *)dest, width, (uint16_t)code);
	} else {
		fill_n((uint8_t *)dest, width, (uint8_t)code);
	}
}

int draw_canvas_raw_buffer2(Canvas_Attr &canvas, vector<DRAW_OBJ> obj_vec,
			    uint8_t *obuf)
{
	size_t pixel_sz = osd_cmpr_get_pixel_sz(canvas.format);
	//memset(obuf, 0, canvas.height*canvas.width*pixel_sz);
	for (int obj_i = 0; obj_i < obj_vec.size(); obj_i++) {
		DRAW_OBJ &obj_attr = obj_vec[obj_i];
		if (obj_attr.type == STROKE_RECT) {
			uint8_t *dest_ptr;
			for (int y = obj_attr.rect.y;
			     y < (obj_attr.rect.y + obj_attr.rect.thickness);
			     y++) {
				uint8_t *dest_ptr = &obuf[(y * canvas.width +
							   obj_attr.rect.x) *
							  pixel_sz];
				fill_n_pixel(dest_ptr, obj_attr.rect.width,
					     obj_attr.color.code, pixel_sz);
			}
			int y2 = obj_attr.rect.y + obj_attr.rect.height -
				 obj_attr.rect.thickness;
			for (int y = y2;
			     y < (obj_attr.rect.y + obj_attr.rect.height);
			     y++) {
				uint8_t *dest_ptr = &obuf[(y * canvas.width +
							   obj_attr.rect.x) *
							  pixel_sz];
				fill_n_pixel(dest_ptr, obj_attr.rect.width,
					     obj_attr.color.code, pixel_sz);
			}
			dest_ptr = &obuf[(obj_attr.rect.y * canvas.width +
					  obj_attr.rect.x) *
					 pixel_sz];
			for (int y = obj_attr.rect.y;
			     y < (obj_attr.rect.y + obj_attr.rect.height);
			     y++) {
				fill_n_pixel(dest_ptr, obj_attr.rect.thickness,
					     obj_attr.color.code, pixel_sz);
				dest_ptr += (canvas.width * pixel_sz);
			}
			int x2 = obj_attr.rect.x + obj_attr.rect.width -
				 obj_attr.rect.thickness;
			dest_ptr = &obuf[(obj_attr.rect.y * canvas.width + x2) *
					 pixel_sz];
			for (int y = obj_attr.rect.y;
			     y < (obj_attr.rect.y + obj_attr.rect.height);
			     y++) {
				fill_n_pixel(dest_ptr, obj_attr.rect.thickness,
					     obj_attr.color.code, pixel_sz);
				dest_ptr += (canvas.width * pixel_sz);
			}
		} else if (obj_attr.type == RECT) {
			uint8_t *dest_ptr =
				&obuf[(obj_attr.rect.y * canvas.width +
				       obj_attr.rect.x) *
				      pixel_sz];
			for (int y = 0; y < obj_attr.rect.height; y++) {
				fill_n_pixel(dest_ptr, obj_attr.rect.width,
					     obj_attr.color.code, pixel_sz);
				dest_ptr += (canvas.width * pixel_sz);
			}
		} else if (obj_attr.type == BIT_MAP) {
			uint8_t *dest_ptr =
				&obuf[(obj_attr.bitmap.rect.y * canvas.width +
				       obj_attr.bitmap.rect.x) *
				      pixel_sz];
			uint8_t *src_ptr = obj_attr.color.buf;
			for (int y = 0; y < obj_attr.rect.height; y++) {
				memcpy(dest_ptr, src_ptr,
				       obj_attr.rect.width * pixel_sz);
				dest_ptr += (canvas.width * pixel_sz);
				src_ptr +=
					(obj_attr.bitmap.rect.width * pixel_sz);
			}
		}
	}
}

void draw_canvas_raw_line(uint8_t *obuf, Canvas_Attr &canvas,
			  vector<DRAW_OBJ> &obj_vec, int y, int pixel_sz,
			  vector<SEGMENT> &segment_vec)
{
	uint8_t *dest_ptr = obuf + (y * canvas.width * pixel_sz);
	for (int seg_i = 0; seg_i < segment_vec.size(); seg_i++) {
		SEGMENT &segment = segment_vec[seg_i];
		if (segment.is_const && segment.color.code != 0) {
			fill_n_pixel(dest_ptr, segment.width,
				     segment.color.code, pixel_sz);
		} else if (!segment.is_const) {
			int rep_cnt;
			uint8_t *src_ptr = segment.color.buf;
			if (segment.is_cmpr) {
				int pel_cnt = 0;
				while (pel_cnt < segment.width) {
					rep_cnt = min(src_ptr[0] + 1,
						      segment.width - pel_cnt);
					fill_n_pixel(
						&dest_ptr[pel_cnt * pixel_sz],
						rep_cnt,
						*((uint32_t *)&src_ptr[1]),
						pixel_sz);
					src_ptr += (1 + pixel_sz);
					pel_cnt += rep_cnt;
				}
				segment.color.buf += *(segment.bs_len);
				obj_vec[segment.id].bitmap.bs_offset +=
					*(segment.bs_len);
				segment.bs_len++;
			} else {
				memcpy(dest_ptr, src_ptr,
				       segment.width * pixel_sz);
				segment.color.buf +=
					(segment.stride * pixel_sz);
			}
		}
		dest_ptr += (segment.width * pixel_sz);
	}
}

int draw_canvas_raw_buffer(Canvas_Attr &canvas, vector<DRAW_OBJ> obj_vec,
			   uint8_t *obuf)
{
	size_t pixel_sz = osd_cmpr_get_pixel_sz(canvas.format);
	memset(obuf, 0, canvas.height * canvas.width * pixel_sz);
	int y_step;
	for (int y = 0; y < canvas.height; y += y_step) {
		int next_y = canvas.height;
		vector<SEGMENT> segment_vec;
		plot_segments_on_line(obj_vec, canvas.width, canvas.height, y,
				      pixel_sz, next_y, segment_vec, canvas->bg_color_code);
		y_step = next_y - y;
		for (int line_i = 0; line_i < y_step; line_i++) {
			draw_canvas_raw_line(obuf, canvas, obj_vec, y + line_i,
					     pixel_sz, segment_vec);
		}
		recycle_draw_obj(obj_vec, y + y_step - 1);
	}

	return 1;
}
#endif

int cmpr_bitmap(Canvas_Attr *canvas, uint8_t *ibuf, uint8_t *obuf, int width,
		int height, int buf_size, uint32_t* p_osize)
{
	size_t pixel_sz = osd_cmpr_get_pixel_sz(canvas->format);
	memset(obuf, 0, buf_size);
	uint8_t *cur_ptr = ibuf;
	uint16_t *meta_ptr = (uint16_t *)obuf;
	size_t meta_sz = height << 1;
	uint8_t *bs_ptr = &obuf[meta_sz];
	size_t bs_sz_cnt = meta_sz;
	int rl_pair_sz = (1 + pixel_sz);
	int step;
	for (int y = 0; y < height; y++) {
		int line_byte_cnt = 0;
		for (int x = 0; x < width; x += step) {
			int cnt = count_repeat_pixel(cur_ptr, pixel_sz,
						     width - x);
			step = cnt;
			while (cnt > 0) {
				int new_bs_sz_cnt = bs_sz_cnt + rl_pair_sz;
				if (new_bs_sz_cnt > buf_size) {
					return -1;
				}
				int rl = min(cnt, 256);
				*bs_ptr = (rl - 1);
				memcpy(&bs_ptr[1], cur_ptr, pixel_sz);
				bs_ptr += rl_pair_sz;
				cnt -= rl;
				bs_sz_cnt = new_bs_sz_cnt;
				line_byte_cnt += rl_pair_sz;
			}
			cur_ptr += (step * pixel_sz);
		}
		meta_ptr[y] = line_byte_cnt;
	}
	*p_osize = bs_sz_cnt;

	return 1;
}

uint32_t est_cmpr_canvas_size(Canvas_Attr *canvas, DRAW_OBJ *objs, uint32_t obj_num)
{
	uint8_t *dummy = nullptr;
	uint32_t bs_sz = 0;
	draw_cmpr_canvas(canvas, objs, obj_num, dummy, 0, &bs_sz);

	return ((bs_sz + BUF_GUARD_SIZE) >> 4) << 4;
}

void set_rect_position(RECT_ATTR &rect, Canvas_Attr *canvas, int pt_x, int pt_y,
		       int width, int height)
{
	rect.x = clip(pt_x, 0, canvas->width - 1);
	rect.y = clip(pt_y, 0, canvas->height -1);
	rect.width = clip(width, 0, canvas->width - rect.x);
	rect.height = clip(height, 0, canvas->height - rect.y);
}

void set_rect_obj_attr(DRAW_OBJ *obj_attr, Canvas_Attr *canvas,
		       uint32_t color_code, int pt_x, int pt_y, int width,
		       int height, bool is_filled, int thickness)
{
	obj_attr->type = (is_filled) ? RECT : STROKE_RECT;
	obj_attr->color.code = color_code;
	set_rect_position(obj_attr->rect, canvas, pt_x, pt_y, width, height);
	thickness = clip(thickness, MIN_THICKNESS, MAX_THICKNESS);
	int min_allow_thickness = max((min(obj_attr->rect.height, obj_attr->rect.width)>>1)-1, 0);
	obj_attr->rect.thickness = min(min_allow_thickness, thickness);
	obj_attr->_max_y = obj_attr->rect.y + obj_attr->rect.height;
	obj_attr->_min_y = obj_attr->rect.y;
}

void set_bitmap_obj_attr(DRAW_OBJ *obj_attr, Canvas_Attr *canvas, uint8_t *buf,
			 int pt_x, int pt_y, int width, int height,
			 bool is_cmpr)
{
	if (is_cmpr) {
		obj_attr->type = CMPR_BIT_MAP;
		obj_attr->bitmap.bs_offset = height
					    << 1; // 2B metadata for each line
	} else {
		obj_attr->type = BIT_MAP;
		obj_attr->bitmap.stride = width;
	}
	set_rect_position(obj_attr->bitmap.rect, canvas, pt_x, pt_y, width,
			  height);
	obj_attr->color.buf = buf;
	obj_attr->_max_y = obj_attr->bitmap.rect.y + obj_attr->bitmap.rect.height;
	obj_attr->_min_y = obj_attr->bitmap.rect.y;
}

void set_line_obj_attr(DRAW_OBJ *obj_attr, Canvas_Attr *canvas,
		       uint32_t color_code, int pt_x0, int pt_y0, int pt_x1,
		       int pt_y1, int thickness)
{
	obj_attr->color.code = color_code;
	pt_x0 = clip(pt_x0, 0, canvas->width);
	pt_x1 = clip(pt_x1, 0, canvas->width);
	pt_y0 = clip(pt_y0, 0, canvas->height);
	pt_y1 = clip(pt_y1, 0, canvas->height);
	thickness = clip(thickness, MIN_THICKNESS, MAX_THICKNESS);
	if (pt_x0 == pt_x1 ||
	    pt_y0 == pt_y1) { // horizontal line: degenerate to rect
		int x = min(pt_x0, pt_x1) - (thickness >> 1);
		int y = min(pt_y0, pt_y1) - (thickness >> 1);
		int width = thickness +
			    ((pt_x0 == pt_x1) ?
				     0 :
				     max(pt_x0, pt_x1) - min(pt_x0, pt_x1));
		int height = thickness +
			     ((pt_y0 == pt_y1) ?
				      0 :
				      max(pt_y0, pt_y1) - min(pt_y0, pt_y1));
		set_rect_obj_attr(obj_attr, canvas, color_code, x, y, width,
				  height, true, thickness);
	} else {
		obj_attr->color.code = color_code;
		obj_attr->type = LINE;
		int x[2], y[2];
		bool is_y_incr = (pt_y1 > pt_y0);
		x[0] = (is_y_incr) ? pt_x0 : pt_x1;
		y[0] = (is_y_incr) ? pt_y0 : pt_y1;
		x[1] = (is_y_incr) ? pt_x1 : pt_x0;
		y[1] = (is_y_incr) ? pt_y1 : pt_y0;
		int dx = x[1] - x[0];
		int dy = y[1] - y[0];
		obj_attr->line._mx = (dy != 0) ? dx / ((float)dy) :
						FLT_MAX;
		float thick_offset =
			0.5 * thickness / pow(dx * dx + dy * dy, 0.5);
		float thick_offset_x = thick_offset * dx;
		float thick_offset_y = thick_offset * dy;
		for (int side_i = 0; side_i < 2; side_i++) {
			obj_attr->line._bx[side_i] =
				clip(x[0] + ((dx >= 0) ? thick_offset_y :
							 (-thick_offset_y)),
				     (float)0., (float)canvas->width);
			obj_attr->line._by[side_i] =
				clip(y[0] + ((dx >= 0) ? (-thick_offset_x) :
							 thick_offset_x),
				     (float)0., (float)canvas->height);
			obj_attr->line._ex[side_i] =
				clip(x[1] + ((dx >= 0) ? thick_offset_y :
							 (-thick_offset_y)),
				     (float)0., (float)canvas->width);
			obj_attr->line._ey[side_i] =
				clip(y[1] + ((dx >= 0) ? (-thick_offset_x) :
							 thick_offset_x),
				     (float)0., (float)canvas->height);
			thick_offset_x = -thick_offset_x;
			thick_offset_y = -thick_offset_y;
		}
		obj_attr->line.ts_h =
			(obj_attr->line._bx[0] +
			 obj_attr->line._mx * (obj_attr->line._by[1] -
					      obj_attr->line._by[0])) -
			obj_attr->line._bx[1];
		obj_attr->_min_y =
			round(min(obj_attr->line._by[0], obj_attr->line._by[1]));
		obj_attr->_max_y =
			round(max(obj_attr->line._ey[1], obj_attr->line._ey[1]));
	}
}

//==============================================================================================
//CVI interface
#ifdef __cplusplus
extern "C"
{
#endif

uint32_t CVI_OSDC_est_cmpr_canvas_size(Canvas_Attr *canvas, DRAW_OBJ *objs, uint32_t obj_num)
{
	return est_cmpr_canvas_size(canvas, objs, obj_num);
}

int CVI_OSDC_draw_cmpr_canvas(Canvas_Attr *canvas, DRAW_OBJ *objs, uint32_t obj_num,
				uint8_t *obuf, uint32_t buf_size, uint32_t *p_osize)
{
	return draw_cmpr_canvas(canvas, objs, obj_num, obuf, buf_size, p_osize);
}

void CVI_OSDC_set_rect_obj_attr(Canvas_Attr *canvas, DRAW_OBJ *obj, uint32_t color_code, int pt_x, int pt_y, int width,
				int height, bool is_filled, int thickness)
{
	set_rect_obj_attr(obj, canvas, color_code, pt_x, pt_y, width, height, is_filled, thickness);

}

void CVI_OSDC_set_bitmap_obj_attr(Canvas_Attr *canvas, DRAW_OBJ *obj_attr, uint8_t *buf,
				int pt_x, int pt_y, int width, int height, bool is_cmpr)
{
	set_bitmap_obj_attr(obj_attr, canvas, buf,  pt_x, pt_y, width, height, is_cmpr);
}

void CVI_OSDC_set_line_obj_attr(Canvas_Attr *canvas, DRAW_OBJ *obj, uint32_t color_code,
				int pt_x0, int pt_y0, int pt_x1, int pt_y1, int thickness)
{
	set_line_obj_attr(obj, canvas, color_code, pt_x0, pt_y0, pt_x1, pt_y1, thickness);
}

int CVI_OSDC_cmpr_bitmap(Canvas_Attr *canvas, uint8_t *ibuf, uint8_t *obuf, int width, int height,
				int buf_size, uint32_t *p_osize)
{
	return cmpr_bitmap(canvas, ibuf, obuf, width, height, buf_size, p_osize);
}

#if (CMPR_CANVAS_DBG)
int CVI_OSDC_draw_canvas_raw_buffer(Canvas_Attr &canvas, vector<DRAW_OBJ> obj_vec, uint8_t *obuf)
int CVI_OSDC_draw_canvas_raw_buffer2(Canvas_Attr &canvas, vector<DRAW_OBJ> obj_vec, uint8_t *obuf);
#endif

#ifdef __cplusplus
}
#endif
