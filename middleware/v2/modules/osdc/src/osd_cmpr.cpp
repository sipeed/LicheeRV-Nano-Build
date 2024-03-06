#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "osd_cmpr.h"

// -- arithmetic operation --
uint8_t get_bit_val(uint8_t *buf, int byte_idx, int bit_idx)
{
	return (buf[byte_idx] >> bit_idx) & 0x1;
}

int min(int data1, int data2)
{
	return (data1 <= data2) ? data1 : data2;
}

int max(int data1, int data2)
{
	return (data1 >= data2) ? data1 : data2;
}

int clip(int data, int min, int max)
{
	return (data > max) ? max : (data < min) ? min : data;
}

// -- streaming operation handler --
void init_stream(StreamBuffer *bs, const uint8_t *buf, int buf_size,
		 bool read_only)
{
	bs->bit_pos = 0;
	bs->stream = (uint8_t *)buf;
	bs->buf_size = buf_size;
	bs->status = 1;
	if (!read_only)
		memset((uint8_t *)buf, 0, sizeof(uint8_t) * buf_size);
}

void write_multibits(uint8_t *stream, uint8_t *src, int bit_pos, int bit_len)
{
	assert(bit_len <= 8);
	int dest_bit_i = bit_pos & 7;
	int dest_byte_i = bit_pos >> 3;
	uint16_t *dest_ptr_ex = (uint16_t *)&stream[dest_byte_i];
	uint16_t src_data_ex = (*src) << dest_bit_i;
	(*dest_ptr_ex) = (*dest_ptr_ex) | src_data_ex;
}

void write_stream(StreamBuffer *bs, uint8_t *src, int bit_len)
{
	int next_bit_pos = bs->bit_pos + bit_len;
	if (next_bit_pos < (bs->buf_size << 3)) {
		while (bit_len >= 8) {
			write_multibits(bs->stream, src, bs->bit_pos, 8);
			bs->bit_pos += 8;
			bit_len -= 8;
			src++;
		};
		if (bit_len > 0) {
			write_multibits(bs->stream, src, bs->bit_pos, bit_len);
		}
	} else {
		bs->status = -1;
	}
	bs->bit_pos = next_bit_pos;
}

void move_stream_ptr(StreamBuffer *bs, int bit_len)
{
	bs->bit_pos = min(bs->bit_pos + bit_len, (bs->buf_size << 3));
}

void parse_stream(StreamBuffer *bs, uint8_t *dest, int bit_len, bool read_only)
{
	memset(dest, 0, sizeof(uint8_t) * (bit_len + 7) >> 3);
	for (int bit = 0; bit < bit_len; bit++) {
		int dest_byte_i = bit / 8;
		int dest_bit_i = bit % 8;
		int bs_byte_i = (bs->bit_pos + bit) / 8;
		int bs_bit_i = (bs->bit_pos + bit) % 8;
		dest[dest_byte_i] |=
			(get_bit_val(bs->stream, bs_byte_i, bs_bit_i)
			 << dest_bit_i);
	}
	bs->bit_pos = (read_only) ?
			      bs->bit_pos :
			      min(bs->bit_pos + bit_len, (bs->buf_size << 3));
}

// ------ ARGB format generic function ------

// tranform generic ARGB8888 into multiple pixel format
void set_color(uint8_t *ptr, RGBA color, OSD_FORMAT format)
{
	if (format == OSD_ARGB8888) {
		memcpy(ptr, &color, sizeof(RGBA));
	} else if (format == OSD_ARGB1555) {
		ARGB1555 out_c;
		CPY_C(color, out_c)
		memcpy(ptr, &out_c.code, sizeof(out_c.code));
	} else if (format == OSD_ARGB4444) {
		ARGB4444 out_c;
		CPY_C(color, out_c)
		memcpy(ptr, &out_c.code, sizeof(out_c.code));
	} else if (format == OSD_LUT8 || format == OSD_LUT4) {
		*ptr = color.a;
	}
}

// tranform multiple pixel format into generic ARGB8888
RGBA get_color(uint8_t *ptr, OSD_FORMAT format)
{
	RGBA out_c;
	out_c.code = 0;
	if (format == OSD_ARGB8888) {
		RGBA in_c = *((RGBA *)ptr);
		CPY_C(in_c, out_c)
	} else if (format == OSD_ARGB1555) {
		ARGB1555 in_c = *((ARGB1555 *)ptr);
		CPY_C(in_c, out_c)
	} else if (format == OSD_ARGB4444) {
		ARGB4444 in_c = *((ARGB4444 *)ptr);
		CPY_C(in_c, out_c)
	} else if (format == OSD_LUT8 || format == OSD_LUT4) {
		out_c.a = *ptr;
	}

	return out_c;
}

bool is_equal_color(RGBA c0, RGBA c1)
{
	return c0.code == c1.code;
}

// ------ palette_cache ------
void palette_cache_init(vector<RGBA> &cache, int cache_sz)
{
	int c_incr = 256 / cache_sz;
	uint8_t c = 0;
	for (int idx = 0; idx < cache_sz; idx++) {
		uint32_t code = (c << 24) | (c << 16) | (c << 8) | c;
		RGBA color = get_color((uint8_t *)&code);
		cache.push_back(color);
		c = clip(c + c_incr, 0, 255);
	}
}

int palette_cache_lookup_color(vector<RGBA> &cache, RGBA color)
{
	for (unsigned int idx = 0; idx < cache.size(); idx++) {
		if (is_equal_color(cache[idx], color)) { // hit
			palette_cache_lru_update(cache, idx);
			return idx;
		}
	}
	//  miss
	palette_cache_push_color(cache, color);

	return -1;
}

void palette_cache_lru_update(vector<RGBA> &cache, int index)
{
	if (index == 0)
		return;
	RGBA reg_color = cache[index];
	for (int idx = index; idx > 0; idx--) {
		cache[idx] = cache[idx - 1];
	}
	cache[0] = reg_color;
}

void palette_cache_push_color(vector<RGBA> &cache, RGBA color)
{
	for (int idx = cache.size() - 1; idx > 0; idx--) {
		cache[idx] = cache[idx - 1];
	}
	cache[0] = color;
}

// ------ syntax enc/dec ------
void enc_literal(StreamBuffer *bs, RGBA color, OSD_FORMAT format)
{
	if (format == OSD_ARGB8888) {
		write_stream(bs, (uint8_t *)(&color), sizeof(RGBA) << 3);
	} else if (format == OSD_ARGB1555) {
		ARGB1555 out_c;
		CPY_C(color, out_c)
		write_stream(bs, (uint8_t *)(&out_c.code),
			     sizeof(out_c.code) << 3);
	} else if (format == OSD_ARGB4444) {
		ARGB4444 out_c;
		CPY_C(color, out_c)
		write_stream(bs, (uint8_t *)(&out_c.code),
			     sizeof(out_c.code) << 3);
	} else if (format == OSD_LUT8) {
		write_stream(bs, (uint8_t *)(&color.a), 8);
	} else if (format == OSD_LUT4) {
		write_stream(bs, (uint8_t *)(&color.a), 4);
	}
}

RGBA dec_literal(StreamBuffer *bs, OSD_FORMAT format)
{
	RGBA color;
	color.code = 0;
	if (format == OSD_ARGB8888) {
		parse_stream(bs, (uint8_t *)&color, sizeof(RGBA) * 8, false);
	} else if (format == OSD_ARGB1555) {
		ARGB1555 in_c;
		parse_stream(bs, (uint8_t *)&in_c.code, sizeof(in_c.code) * 8,
			     false);
		CPY_C(in_c, color)
	} else if (format == OSD_ARGB4444) {
		ARGB4444 in_c;
		parse_stream(bs, (uint8_t *)&in_c.code, sizeof(in_c.code) * 8,
			     false);
		CPY_C(in_c, color)
	} else if (format == OSD_LUT8) {
		parse_stream(bs, &color.a, 8, false);
	} else if (format == OSD_LUT4) {
		parse_stream(bs, &color.a, 4, false);
	}

	return color;
}

void enc_run_length(StreamBuffer *bs, int run_len, int run_len_bd)
{
	if (run_len > 1) {
		uint8_t run_syntax = run_len - 1;
		write_stream(bs, &run_syntax, run_len_bd);
	}
}

void enc_mode_syntax(StreamBuffer *bs, MODE_TYPE md, int run_len, CODE code,
		     OSDCmpr_Ctrl *p_ctrl)
{
	if (p_ctrl->reg_palette_mode_en) {
		uint16_t _syntax;
		if (md == Palette) {
			if (run_len > 1) {
				_syntax = (code.palette_idx
					   << (p_ctrl->reg_run_len_bd + 3)) |
					  ((run_len - 1) << 3);
				write_stream(
					bs, (uint8_t *)&_syntax,
					3 + p_ctrl->reg_run_len_bd +
						p_ctrl->reg_palette_idx_bd);
			} else {
				_syntax = (code.palette_idx << 2) | 2;
				write_stream(bs, (uint8_t *)&_syntax,
					     p_ctrl->reg_palette_idx_bd + 2);
			}
		} else if (md == Literal) {
			if (run_len > 1) {
				uint16_t _syntax = ((run_len - 1) << 3) | 4;
				write_stream(bs, (uint8_t *)&_syntax,
					     3 + p_ctrl->reg_run_len_bd);
			} else {
				uint8_t lit_prefix = 1;
				write_stream(bs, &lit_prefix, 1);
			}
			enc_literal(bs, code.color, p_ctrl->reg_osd_format);
		}
	} else {
		uint8_t prefix = (run_len > 1) ? 0 : 1;
		write_stream(bs, &prefix, 1);
		enc_run_length(bs, run_len, p_ctrl->reg_run_len_bd);
		enc_literal(bs, code.color, p_ctrl->reg_osd_format);
	}
}

MODE_TYPE dec_mode_prefix(StreamBuffer *bs, bool palette_mode_en)
{
	uint8_t prefix;
	if (palette_mode_en) {
		parse_stream(bs, &prefix, 3, true);
		if (prefix == 0) {
			move_stream_ptr(bs, 3);
			return Palette_RL;
		} else if (prefix == 4) {
			move_stream_ptr(bs, 3);
			return Literal_RL;
		} else if ((prefix & 0x3) == 2) {
			move_stream_ptr(bs, 2);
			return Palette;
		}
		assert((prefix & 0x1) == 1);
		move_stream_ptr(bs, 1);
		return Literal;
	} else {
		parse_stream(bs, &prefix, 1, false);
		return (prefix) ? Literal : Literal_RL;
	}
}

CODE dec_code_syntax(StreamBuffer *bs, MODE_TYPE md, OSDCmpr_Ctrl *p_ctrl)
{
	CODE retCode;
	if (md == Literal || md == Literal_RL) {
		retCode.color = dec_literal(bs, p_ctrl->reg_osd_format);
	} else {
		uint8_t pal_syntax;
		parse_stream(bs, &pal_syntax, p_ctrl->reg_palette_idx_bd,
			     false);
		retCode.palette_idx = pal_syntax;
	}

	return retCode;
}

RGBA pixel_preprocess(uint8_t *ptr, OSDCmpr_Ctrl *p_ctrl)
{
	RGBA color = get_color(ptr, p_ctrl->reg_osd_format);
	color.b = (color.b >> p_ctrl->reg_rgb_trunc_bit)
		  << p_ctrl->reg_rgb_trunc_bit;
	color.g = (color.g >> p_ctrl->reg_rgb_trunc_bit)
		  << p_ctrl->reg_rgb_trunc_bit;
	color.r = (color.r >> p_ctrl->reg_rgb_trunc_bit)
		  << p_ctrl->reg_rgb_trunc_bit;
	color.a = (color.a >> p_ctrl->reg_alpha_trunc_bit)
		  << p_ctrl->reg_alpha_trunc_bit;
	if (p_ctrl->reg_zeroize_by_alpha && color.a == 0) {
		color.code = 0;
	}
	set_color(ptr, color, p_ctrl->reg_osd_format);

	return color;
}

// ---------------------- OSD cmpr main API ----------------------
int osd_cmpr_enc_one_frame(uint8_t *ibuf, uint8_t *obs, OSDCmpr_Ctrl *p_ctrl)
{
	StreamBuffer bitstream;
	size_t width = p_ctrl->reg_image_width,
	       height = p_ctrl->reg_image_height;
	init_stream(&bitstream, p_ctrl->bsbuf, p_ctrl->bs_buf_size, false);
	uint8_t *inPtr = ibuf;
	RGBA last_color;
	int rl_cnt = 1;
	CODE code;
	MODE_TYPE md = NUM_OF_MODE;
	int max_run_len = 1 << p_ctrl->reg_run_len_bd;
	for (int line_i = 0; line_i < (int)height; line_i++) {
		for (int pos_x = 0; pos_x < (int)width; pos_x++) {
			RGBA cur = pixel_preprocess(inPtr, p_ctrl);
			if ((pos_x == 0 && line_i == 0) ||
			    rl_cnt >= max_run_len ||
			    (!is_equal_color(cur, last_color))) { // new run
				if (!(pos_x == 0 && line_i == 0)) { // write out
					enc_mode_syntax(&bitstream, md, rl_cnt,
							code, p_ctrl);
				}
				// mode detection
				int cache_idx =
					(p_ctrl->reg_palette_mode_en) ?
						palette_cache_lookup_color(
							p_ctrl->palette_cache,
							cur) :
						-1;
				if (cache_idx >= 0) { // cache hit
					code.palette_idx = cache_idx;
					md = Palette;
				} else { // cache miss
					code.color = cur;
					md = Literal;
				}
				rl_cnt = 1;
			} else { // still within a run
				rl_cnt++;
			}
			last_color = cur;
			inPtr += p_ctrl->pel_sz;
		}
	}
	enc_mode_syntax(&bitstream, md, rl_cnt, code, p_ctrl);
	int blk_bs_size = ((bitstream.bit_pos + 127) >> 7)
			  << 4; // in byte, 16byte align
	memcpy(obs, p_ctrl->bsbuf, blk_bs_size * sizeof(uint8_t));

	return blk_bs_size;
}

void osd_cmpr_dec_one_frame(uint8_t *bsbuf, size_t bs_size, uint8_t *obuf,
			    OSDCmpr_Ctrl *p_ctrl)
{
	StreamBuffer bitstream;
	init_stream(&bitstream, bsbuf, bs_size, true);
	int remain_pix_cnt = p_ctrl->reg_image_width * p_ctrl->reg_image_height;
	uint8_t *outPtr = obuf;
	int run_len;
	RGBA color;
	while (remain_pix_cnt > 0) {
		MODE_TYPE md = dec_mode_prefix(&bitstream,
					       p_ctrl->reg_palette_mode_en);
		// decode run len
		if (md == Literal_RL || md == Palette_RL) {
			uint8_t run_syntax = 0;
			parse_stream(&bitstream, &run_syntax,
				     p_ctrl->reg_run_len_bd, false);
			run_len = run_syntax + 1;
		} else {
			run_len = 1;
		}

		CODE code = dec_code_syntax(&bitstream, md, p_ctrl);
		if (md == Literal || md == Literal_RL) {
			color = code.color;
			palette_cache_push_color(p_ctrl->palette_cache, color);
		} else if (md == Palette || md == Palette_RL) {
			color = p_ctrl->palette_cache[code.palette_idx];
			palette_cache_lru_update(p_ctrl->palette_cache,
						 code.palette_idx);
		}

		// reconstruct pixels
		for (int idx = 0; idx < run_len; idx++) {
			set_color(outPtr, color, p_ctrl->reg_osd_format);
			outPtr += p_ctrl->pel_sz;
			remain_pix_cnt--;
		}
	}
}

void osd_cmpr_enc_header(uint8_t *hdrbuf, OSDCmpr_Ctrl *p_ctrl)
{
	StreamBuffer bs_header;
	size_t width_m1 = p_ctrl->reg_image_width - 1,
	       height_m1 = p_ctrl->reg_image_height - 1;
	init_stream(&bs_header, hdrbuf, HDR_SZ, false);
	move_stream_ptr(&bs_header, 8); // bit[0:7] version
	write_stream(&bs_header, (uint8_t *)&p_ctrl->reg_osd_format,
		     4); // bit[8:11] osd_format
	move_stream_ptr(&bs_header, 3); // bit[12:14] reserved
	size_t palette_cache_size = 1 << p_ctrl->reg_palette_idx_bd;
	write_stream(&bs_header, (uint8_t *)&palette_cache_size,
		     8); // bit[15:22] palette_cache_size
	write_stream(&bs_header, (uint8_t *)&p_ctrl->reg_alpha_trunc_bit,
		     2); // bit[23:24] alpha truncate
	move_stream_ptr(&bs_header, 2); // bit[25:26] reserved
	write_stream(&bs_header, (uint8_t *)&p_ctrl->reg_rgb_trunc_bit,
		     2); // bit[27:28] alpha truncate
	move_stream_ptr(&bs_header, 2); // bit[29:30] reserved
	write_stream(&bs_header, (uint8_t *)&width_m1,
		     16); // bit[31:46] image_width minus 1
	write_stream(&bs_header, (uint8_t *)&height_m1,
		     16); // bit[47:62] image_height minus 1
}

void osd_cmpr_dec_header(uint8_t *hdrbuf, OSDCmpr_Ctrl *p_ctrl)
{
	StreamBuffer bs_header;
	init_stream(&bs_header, hdrbuf, HDR_SZ, true);
	uint8_t palette_cache_size;
	uint16_t width_m1, height_m1;
	p_ctrl->reg_osd_format = OSD_ARGB8888;
	p_ctrl->reg_alpha_trunc_bit = 0;
	p_ctrl->reg_rgb_trunc_bit = 0;

	move_stream_ptr(&bs_header, 8); // bit[0:7] version
	parse_stream(&bs_header, (uint8_t *)&p_ctrl->reg_osd_format, 4,
		     false); // bit[8:11] osd_format
	move_stream_ptr(&bs_header, 3); // bit[12:14] reserved
	parse_stream(&bs_header, &palette_cache_size, 8,
		     false); // bit[15:22] palette_cache_size
	parse_stream(&bs_header, (uint8_t *)&p_ctrl->reg_alpha_trunc_bit, 2,
		     false); // bit[23:24] alpha truncate
	move_stream_ptr(&bs_header, 2); // bit[25:26] reserved
	parse_stream(&bs_header, (uint8_t *)&p_ctrl->reg_rgb_trunc_bit, 2,
		     false); // bit[27:28] alpha truncate
	move_stream_ptr(&bs_header, 2); // bit[29:30] reserved
	parse_stream(&bs_header, (uint8_t *)&width_m1, 16,
		     false); // bit[31:46] image_width minus 1
	parse_stream(&bs_header, (uint8_t *)&height_m1, 16,
		     false); // bit[47:62] image_height minus 1
	p_ctrl->reg_palette_mode_en = palette_cache_size > 1;
	if (p_ctrl->reg_palette_mode_en) {
		int palette_idx_bd = 0;
		while ((1 << palette_idx_bd) < palette_cache_size) {
			palette_idx_bd++;
		}
		p_ctrl->reg_palette_idx_bd = palette_idx_bd;
	}
	p_ctrl->reg_image_width = width_m1 + 1;
	p_ctrl->reg_image_height = height_m1 + 1;
}

void osd_cmpr_enc_followed_run(RGBA cur_c, int &rl_cnt, MODE_TYPE &md,
			       CODE &code, int &length, int max_run_len,
			       OSDCmpr_Ctrl *p_ctrl, StreamBuffer *bitstream)
{
	enc_mode_syntax(bitstream, md, rl_cnt, code, p_ctrl);
	rl_cnt = min(length, max_run_len);
	length -= rl_cnt;
	// followed run must select Palette idx 0
	if (md != Palette || (md == Palette && code.palette_idx != 0)) {
		if (p_ctrl->reg_palette_mode_en) {
			code.palette_idx = 0;
			md = Palette;
		} else {
			code.color = cur_c;
			md = Literal;
		}
	}
}

void osd_cmpr_enc_const_pixel(RGBA cur_c, RGBA &last_c, int &rl_cnt,
			      MODE_TYPE &md, CODE &code, int &length,
			      bool is_force_new_run, int max_run_len,
			      OSDCmpr_Ctrl *p_ctrl, StreamBuffer *bitstream)
{
	if ((!is_equal_color(cur_c, last_c)) || is_force_new_run ||
	    (rl_cnt == max_run_len)) {
		// new run
		enc_mode_syntax(bitstream, md, rl_cnt, code, p_ctrl);
		// mode detection
		int cache_idx = (p_ctrl->reg_palette_mode_en) ?
					palette_cache_lookup_color(
						p_ctrl->palette_cache, cur_c) :
					-1;
		if (cache_idx >= 0) { // cache hit
			code.palette_idx = cache_idx;
			md = Palette;
		} else { // cache miss
			code.color = cur_c;
			md = Literal;
		}
		rl_cnt = min(length, max_run_len);
		length -= rl_cnt;
		last_c = cur_c;
	} else { // still within a run
		int new_rl_cnt = min(rl_cnt + length, max_run_len);
		length -= (new_rl_cnt - rl_cnt);
		rl_cnt = new_rl_cnt;
	}
}

void osd_cmpr_debug_frame_compare(OSDCmpr_Ctrl *p_ctrl, uint8_t *buf0,
				  uint8_t *buf1)
{
	int frame_pel_num = p_ctrl->reg_image_width * p_ctrl->reg_image_height;
	uint8_t *ptr0 = buf0, *ptr1 = buf1;
	for (int pel_i = 0; pel_i < frame_pel_num; pel_i++) {
		RGBA color0 = get_color(ptr0, p_ctrl->reg_osd_format);
		RGBA color1 = get_color(ptr1, p_ctrl->reg_osd_format);
		if (!is_equal_color(color0, color1)) {
			printf("pel idx %d %d %d\n", pel_i, color0.code,
			       color1.code);
			break;
		}
		ptr0 += p_ctrl->pel_sz;
		ptr1 += p_ctrl->pel_sz;
	}
}

void osd_cmpr_frame_init(OSDCmpr_Ctrl *p_ctrl)
{
	p_ctrl->palette_cache.clear();
	palette_cache_init(p_ctrl->palette_cache,
			   1 << p_ctrl->reg_palette_idx_bd);
}

void osd_cmpr_setup(OSDCmpr_Ctrl *p_ctrl, OSDCmpr_Cfg *p_cfg)
{
	p_ctrl->reg_image_width = p_cfg->img_width;
	p_ctrl->reg_image_height = p_cfg->img_height;
	p_ctrl->reg_zeroize_by_alpha = p_cfg->zeroize_by_alpha;
	p_ctrl->reg_rgb_trunc_bit = p_cfg->rgb_trunc_bit;
	p_ctrl->reg_alpha_trunc_bit = p_cfg->alpha_trunc_bit;
	p_ctrl->reg_palette_mode_en = p_cfg->palette_mode_en;
	p_ctrl->reg_run_len_bd = p_cfg->run_len_bd;
	p_ctrl->reg_palette_idx_bd =
		(p_cfg->palette_mode_en) ? p_cfg->palette_idx_bd : 0;
	p_ctrl->reg_osd_format = p_cfg->osd_format;
	p_ctrl->pel_sz = osd_cmpr_get_pixel_sz(p_cfg->osd_format);
	p_ctrl->bs_buf_size = osd_cmpr_get_bs_buf_max_sz(
		p_cfg->img_width * p_cfg->img_height, p_ctrl->pel_sz);
	p_ctrl->bsbuf = (uint8_t *)calloc(p_ctrl->bs_buf_size, sizeof(uint8_t));
}

size_t osd_cmpr_get_pixel_sz(OSD_FORMAT format)
{
	return (format == OSD_ARGB8888) ?
		       4 :
		       (format == OSD_ARGB1555 || format == OSD_ARGB4444) ?
		       2 :
		       1; // (OSD_LUT8, OSD_LUT4)
};

size_t osd_cmpr_get_bs_buf_max_sz(int pel_num, int pel_sz)
{
	return HDR_SZ + ((((pel_num * (pel_sz * 8 + 1)) + 127) >> 7)
			 << 4); // in bytes, 16byte align
}

size_t osd_cmpr_get_header_sz()
{
	return HDR_SZ;
}
