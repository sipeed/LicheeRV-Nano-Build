#include <cvi_osdc.h>

uint32_t CVI_OSDC_EstCmprCanvasSize(OSDC_Canvas_Attr_S *canvas, OSDC_DRAW_OBJ_S *objs, uint32_t obj_num)
{
	return CVI_OSDC_est_cmpr_canvas_size(canvas, objs, obj_num);
}

int CVI_OSDC_DrawCmprCanvas(OSDC_Canvas_Attr_S *canvas, OSDC_DRAW_OBJ_S *objs, uint32_t obj_num,
				uint8_t *obuf, uint32_t buf_size, uint32_t *p_osize)
{
	return CVI_OSDC_draw_cmpr_canvas(canvas, objs, obj_num, obuf, buf_size, p_osize);
}

void CVI_OSDC_SetRectObjAttr(OSDC_Canvas_Attr_S *canvas, OSDC_DRAW_OBJ_S *obj, uint32_t color_code,
				int pt_x, int pt_y, int width, int height, bool is_filled, int thickness)
{
	CVI_OSDC_set_rect_obj_attr(canvas, obj, color_code, pt_x, pt_y, width, height, is_filled, thickness);

}

void CVI_OSDC_SetBitmapObjAttr(OSDC_Canvas_Attr_S *canvas, OSDC_DRAW_OBJ_S *obj_attr, uint8_t *buf,
				int pt_x, int pt_y, int width, int height, bool is_cmpr)
{
	CVI_OSDC_set_bitmap_obj_attr(canvas, obj_attr, buf,  pt_x, pt_y, width, height, is_cmpr);
}

void CVI_OSDC_SetLineObjAttr(OSDC_Canvas_Attr_S *canvas, OSDC_DRAW_OBJ_S *obj, uint32_t color_code,
				int pt_x0, int pt_y0, int pt_x1, int pt_y1, int thickness)
{
	CVI_OSDC_set_line_obj_attr(canvas, obj, color_code, pt_x0, pt_y0, pt_x1, pt_y1, thickness);
}

int CVI_OSDC_CmprBitmap(OSDC_Canvas_Attr_S *canvas, uint8_t *ibuf, uint8_t *obuf, int width, int height,
				int buf_size, uint32_t *p_osize)
{
	return CVI_OSDC_cmpr_bitmap(canvas, ibuf, obuf, width, height, buf_size, p_osize);
}
