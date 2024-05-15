#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/prctl.h>
#include <inttypes.h>

#include "cvi_buffer.h"
#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_comm_isp.h"
#include "cvi_ive.h"
#include "cvi_awb.h"

#include "sample_comm.h"
#include "sophgo_middleware.h"
// #include "vo_uapi.h"
#include "rtsp-server.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>


// #define DEBUG_EN
#ifdef DEBUG_EN
#define DEBUG(fmt, args...) printf("[%s][%d]: "fmt, __func__, __LINE__, ##args)
#else
#define DEBUG(fmt, args...)
#endif

int exit_flag = 0;
static void sig_handle(CVI_S32 signo)
{
	UNUSED(signo);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	exit_flag = 1;
}

static void exit_handle(CVI_S32 signo)
{
	printf("exit handle! signo:%d\n", signo);
	mmf_deinit();
	exit(0);
}

static uint64_t _get_time_us(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec + tv.tv_sec * 1000000;
}

static int save_buff_to_file(char *filename, uint8_t *filebuf, uint32_t filebuf_len)
{
    int fd = -1;
    fd = open(filename, O_WRONLY | O_CREAT, 0777);
    if (fd <= 2) {
        DEBUG("Open filed, fd = %d\r\n", fd);
        return -1;
    }

    int res = 0;
    if ((res = write(fd, filebuf, filebuf_len)) < 0) {
        DEBUG("Write failed");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

uint8_t *load_file_to_dyna_mem(const char *filename, uint8_t **filebuf, uint32_t *filelen)
{
    struct stat statbuf;
    stat(filename, &statbuf);
    DEBUG("file:%s  size:%ld\r\n", filename, statbuf.st_size);
    uint8_t *buf = malloc(statbuf.st_size);
    if (!buf) {
        DEBUG("Malloc failed!\n");
        goto _err;
    }
    int fd = -1;
    fd = open(filename, O_RDONLY);
    if (fd <= 2) {
        DEBUG("Open filed %d\r\n", fd);
        goto _err;
    }

    int res = 0;
    if ((res = read(fd, buf, statbuf.st_size)) < 0) {
        perror("Read failed");
        close(fd);
        goto _err;
    }
    close(fd);

    if (filebuf)
        *filebuf = buf;
    if (filelen)
        *filelen = statbuf.st_size;
    return buf;
_err:
    if (buf)
        free(buf);
    return NULL;
}

static void _rgb888_to_nv21(uint8_t* data, Uint32 w, Uint32 h, uint8_t* yuv)
{
    Uint32 row_bytes;
    uint8_t* uv;
    uint8_t* y;
    uint8_t r, g, b;
    uint8_t y_val, u_val, v_val;

    uint8_t * img;
    Uint32 i, j;
    y = yuv;
    uv = yuv + w * h;

    row_bytes = (w * 3 );
    h = h & ~1;
    //先转换Y
    for (i = 0; i < h; i++)
    {
        img = data + row_bytes * i;
        for (j = 0; j <w; j++)
        {
            r = *(img+3*j);
            g = *(img+3*j+1);
            b = *(img+3*j+2);
            if(r>=254&&g>=254&&b>=254)
            {
    	        y_val=254;
    	        *y++ = y_val;
   	        continue;
            }
           y_val = (uint8_t)(((int)(299 * r) + (int)(597 * g) + (int)(114 * b)) / 1000);
           *y++ = y_val;
        }
    }
    //转换uv
    for (i = 0; i <h; i += 2)
    {
	img = data + row_bytes * i;
 	for (j = 0; j < w; j+=2)
  	{
	    r = *(img+3*j);
	    g = *(img+3*j+1);
	    b = *(img+3*j+2);
	    u_val= (uint8_t)(((int)(-168.7 * r) - (int)(331.3 * g) + (int)(500 * b) + 128000) / 1000);
	    v_val= (uint8_t)(((int)(500 * r) - (int)(418.7 * g) - (int)(81.3 * b) + 128000) / 1000);
	    *uv++ = v_val;
	    *uv++ = u_val;
	}
   }
}

// static void _rgb8888_to_rgb888(uint8_t *src, uint8_t *dst, int width, int height)
// {
// 	for (int i = 0; i < height; i ++) {
// 		for (int j = 0; j < width; j ++) {
// 			dst[(i * width + j) * 3 + 0] = src[(i * width + j) * 4 + 0];
// 			dst[(i * width + j) * 3 + 1] = src[(i * width + j) * 4 + 1];
// 			dst[(i * width + j) * 3 + 2] = src[(i * width + j) * 4 + 2];
// 		}
// 	}
// }

// // rgb888 to gray
// static void _rgb888_to_gray(uint8_t *src, uint8_t *dst, int width, int height)
// {
// 	for (int i = 0; i < height; i ++) {
// 		for (int j = 0; j < width; j ++) {
// 			dst[i * width + j] = (src[(i * width + j) * 3 + 0] * 38 + src[(i * width + j) * 3 + 1] * 75 + src[(i * width + j) * 3 + 2] * 15) >> 7;
// 		}
// 	}
// }


static uint8_t *_prepare_image(int width, int height, int format)
{
	switch (format) {
	case PIXEL_FORMAT_RGB_888:
	{
		uint8_t *rgb_data = (uint8_t *)malloc(width * height * 3);
		int x_oft = 0;
		int remain_width = width;
		int segment_width = width / 6;
		int idx = 0;
		while (remain_width > 0) {
			int seg_w = (remain_width > segment_width) ? segment_width : remain_width;
			uint8_t r,g,b;
			switch (idx) {
			case 0: r = 0xff, g = 0x00, b = 0x00; break;
			case 1: r = 0x00, g = 0xff, b = 0x00; break;
			case 2: r = 0x00, g = 0x00, b = 0xff; break;
			case 3: r = 0xff, g = 0xff, b = 0x00; break;
			case 4: r = 0xff, g = 0x00, b = 0xff; break;
			case 5: r = 0x00, g = 0xff, b = 0xff; break;
			default: r = 0x00, g = 0x00, b = 0x00; break;
			}
			idx ++;
			for (int i = 0; i < height; i ++) {
				for (int j = 0; j < seg_w; j ++) {
					rgb_data[(i * width + x_oft + j) * 3 + 0] = r;
					rgb_data[(i * width + x_oft + j) * 3 + 1] = g;
					rgb_data[(i * width + x_oft + j) * 3 + 2] = b;
				}
			}
			x_oft += seg_w;
			remain_width -= seg_w;
		}

		for (int i = 0; i < height; i ++) {
			uint8_t *buff = &rgb_data[(i * width + i) * 3];
			buff[0] = 0xff;
			buff[1] = 0xff;
			buff[2] = 0xff;
		}
		for (int i = 0; i < height; i ++) {
			uint8_t *buff = &rgb_data[(i * width + i + width - height) * 3];
			buff[0] = 0xff;
			buff[1] = 0xff;
			buff[2] = 0xff;
		}

		return rgb_data;
	}
	case PIXEL_FORMAT_ARGB_8888:
	{
		uint8_t *rgb_data = (uint8_t *)malloc(width * height * 4);
		memset(rgb_data, 0x00, width * height * 4);
		int x_oft = 0;
		int remain_width = width;
		int segment_width = width / 6;
		int idx = 0;
		while (remain_width > 0) {
			int seg_w = (remain_width > segment_width) ? segment_width : remain_width;
			uint8_t r,g,b,a;
			switch (idx) {
			case 0: r = 0xff, g = 0x00, b = 0x00; a = 0x10; break;
			case 1: r = 0x00, g = 0xff, b = 0x00; a = 0x20; break;
			case 2: r = 0x00, g = 0x00, b = 0xff; a = 0x40; break;
			case 3: r = 0xff, g = 0xff, b = 0x00; a = 0x60; break;
			case 4: r = 0xff, g = 0x00, b = 0xff; a = 0x80; break;
			case 5: r = 0x00, g = 0xff, b = 0xff; a = 0xA0; break;
			default: r = 0x00, g = 0x00, b = 0x00; a = 0xC0; break;
			}
			idx ++;
			for (int i = 0; i < height; i ++) {
				for (int j = 0; j < seg_w; j ++) {
					rgb_data[(i * width + x_oft + j) * 4 + 0] = r;
					rgb_data[(i * width + x_oft + j) * 4 + 1] = g;
					rgb_data[(i * width + x_oft + j) * 4 + 2] = b;
					rgb_data[(i * width + x_oft + j) * 4 + 3] = a;
				}
			}
			x_oft += seg_w;
			remain_width -= seg_w;
		}

		// for (int i = 0; i < height; i ++) {
		// 	uint8_t *buff = &rgb_data[(i * width + i) * 4];
		// 	buff[0] = 0xff;
		// 	buff[1] = 0xff;
		// 	buff[2] = 0xff;
		// }
		// for (int i = 0; i < height; i ++) {
		// 	uint8_t *buff = &rgb_data[(i * width + i + width - height) * 4];
		// 	buff[0] = 0xff;
		// 	buff[1] = 0xff;
		// 	buff[2] = 0xff;
		// }

		return rgb_data;
	}
	case PIXEL_FORMAT_NV21:
	{
		uint8_t *rgb_data = (uint8_t *)malloc(width * height * 3);
		int x_oft = 0;
		int remain_width = width;
		int segment_width = width / 6;
		int idx = 0;
		while (remain_width > 0) {
			int seg_w = (remain_width > segment_width) ? segment_width : remain_width;
			uint8_t r,g,b;
			switch (idx) {
			case 0: r = 0xff, g = 0x00, b = 0x00; break;
			case 1: r = 0x00, g = 0xff, b = 0x00; break;
			case 2: r = 0x00, g = 0x00, b = 0xff; break;
			case 3: r = 0xff, g = 0xff, b = 0x00; break;
			case 4: r = 0xff, g = 0x00, b = 0xff; break;
			case 5: r = 0x00, g = 0xff, b = 0xff; break;
			default: r = 0x00, g = 0x00, b = 0x00; break;
			}
			idx ++;
			for (int i = 0; i < height; i ++) {
				for (int j = 0; j < seg_w; j ++) {
					rgb_data[(i * width + x_oft + j) * 3 + 0] = r;
					rgb_data[(i * width + x_oft + j) * 3 + 1] = g;
					rgb_data[(i * width + x_oft + j) * 3 + 2] = b;
				}
			}
			x_oft += seg_w;
			remain_width -= seg_w;
		}

		for (int i = 0; i < height; i ++) {
			uint8_t *buff = &rgb_data[(i * width + i) * 3];
			buff[0] = 0xff;
			buff[1] = 0xff;
			buff[2] = 0xff;
		}
		for (int i = 0; i < height; i ++) {
			uint8_t *buff = &rgb_data[(i * width + i + width - height) * 3];
			buff[0] = 0xff;
			buff[1] = 0xff;
			buff[2] = 0xff;
		}

		uint8_t *nv21 = (uint8_t *)malloc(width * height * 3 / 2);
		_rgb888_to_nv21(rgb_data, width, height, nv21);
		return nv21;
	}
	break;
	default:
		DEBUG("Only support PIXEL_FORMAT_RGB_888\r\n");
		break;
	}
	return NULL;
}

static int _test_vo_only(void)
{
	uint8_t *filebuf = NULL;
	uint32_t filelen;
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

#if 1
	int img_w = 320, img_h = 240, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	// int img_w = 320, img_h = 240, fit = 0, img_fmt = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	filebuf = _prepare_image(img_w, img_h, img_fmt);
	if (img_fmt == PIXEL_FORMAT_RGB_888)
		filelen = img_w * img_h * 3;
	else
		filelen = img_w * img_h * 3 / 2;
	int show_w = 552, show_h = 368;

	DEBUG("in w:%d h:%d fmt:%d\r\n", img_w, img_h, img_fmt);
	DEBUG("out w:%d h:%d fmt:%d\r\n", show_w, show_h, img_fmt);
#endif

	mmf_init();

	int layer = 0;
	int vo_ch = mmf_get_vo_unused_channel(layer);
	if (0 != mmf_add_vo_channel(layer, vo_ch, show_w, show_h, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		exit_flag = 1;
	}

	// save_buff_to_file("640_480_draw.rgb", filebuf, filelen);
	// system("sync");

	while (!exit_flag) {
		struct timeval tv, tv2;
		gettimeofday(&tv, NULL);
		mmf_vo_frame_push(layer, vo_ch, filebuf, filelen, img_w, img_h, img_fmt, fit);
		gettimeofday(&tv2, NULL);
		DEBUG("mmf vo frame push. %ld\r\n", (tv2.tv_usec + tv2.tv_sec * 1000000) - (tv.tv_usec + tv.tv_sec * 1000000));
		usleep(1000 * 1000);
	}

	mmf_del_vo_channel(layer, vo_ch);
	mmf_deinit();
	return 0;
}

static int _test_vi_only(void)
{
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	if (0 != mmf_init()) {
		DEBUG("mmf_init failed!\r\n");
		return -1;
	}

	DEBUG("sensor id: %#x\n", mmf_get_sensor_id());

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	(void)fit;
	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	mmf_del_vi_channel(vi_ch);
	if (0 != mmf_add_vi_channel(vi_ch, img_w , img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	struct timeval tv;
	uint64_t last_loop = 0;

	void *data;
	int data_size, width, height, format;
	while (!exit_flag) {
		if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			gettimeofday(&tv, NULL);
			DEBUG("Pop..width:%d height:%d data_size:%d format:%d\r\n", width, height, data_size, format);
			DEBUG("data:%p loop: %ld\n", data, (tv.tv_usec + tv.tv_sec * 1000000 - last_loop) / 1000);
			last_loop = tv.tv_usec + tv.tv_sec * 1000000;
#if 0
			static int count = 0;
			if (count++ > 10) {
				save_buff_to_file("2560x1440.yuv", data, data_size);
				system("sync");
				mmf_vi_frame_free(vi_ch);
				break;
			}
#endif
			mmf_vi_frame_free(vi_ch);

			// usleep(50 * 1000);
		}
	}

	mmf_del_vi_channel(vi_ch);
	mmf_deinit();
	return 0;
}

// static void dump_ae_attr(ISP_EXPOSURE_ATTR_S *ae_attr)
// {
// 	printf("ae_attr->u8DebugMode: %d\n", ae_attr->u8DebugMode);
// 	printf("ae_attr->bByPass: %d\n", ae_attr->bByPass);
// 	printf("ae_attr->enOpType: %s\n", ae_attr->enOpType ? "manual" : "auto");
// 	if (ae_attr->enOpType) {
// 		printf("ae_attr->stManual.enExpTimeOpType: %s\n", ae_attr->stManual.enExpTimeOpType ? "manual" : "auto");
// 		printf("ae_attr->stManual.enISONumOpType: %s\n", ae_attr->stManual.enISONumOpType ? "manual" : "auto");
// 		printf("ae_attr->stManual.u32ExpTime: %d\n", ae_attr->stManual.u32ExpTime);
// 		printf("ae_attr->stManual.enGainType: %d\n", ae_attr->stManual.enGainType);
// 		printf("ae_attr->stManual.u32ISONum: %d\n", ae_attr->stManual.u32ISONum);
// 	} else {
// 		printf("ae_attr->stAuto.bManualExpValue: %d\n", ae_attr->stAuto.bManualExpValue);
// 		printf("ae_attr->stAuto.u32ExpValue: %d\n", ae_attr->stAuto.u32ExpValue);
// 	}
// }

static void dump_wb_attr(ISP_WB_ATTR_S *wb_attr)
{
	printf("wb_attr->bByPass: %d\n", wb_attr->bByPass);
	printf("wb_attr->enOpType: %s\n", wb_attr->enOpType ? "manual" : "auto");
	if (wb_attr->enOpType == OP_TYPE_MANUAL) {
		printf("wb_attr->stManual.u16Rgain: %d\n", wb_attr->stManual.u16Rgain);
		printf("wb_attr->stManual.u16Grgain: %d\n", wb_attr->stManual.u16Grgain);
		printf("wb_attr->stManual.u16Gbgain: %d\n", wb_attr->stManual.u16Gbgain);
		printf("wb_attr->stManual.u16Bgain: %d\n", wb_attr->stManual.u16Bgain);
	} else if (wb_attr->enOpType == OP_TYPE_AUTO){
		printf("wb_attr->stAuto.bEnable: %d\n", wb_attr->stAuto.bEnable);
		printf("wb_attr->stAuto.u16RefColorTemp: %d\n", wb_attr->stAuto.u16RefColorTemp);
		printf("wb_attr->stAuto.u16Speed: %d\n", wb_attr->stAuto.u16Speed);
		printf("wb_attr->stAuto.u16HighColorTemp: %d\n", wb_attr->stAuto.u16HighColorTemp);
		printf("wb_attr->stAuto.u16LowColorTemp: %d\n", wb_attr->stAuto.u16LowColorTemp);
	}
}

int mmf_get_wb(int ch, uint16_t *wb_value)
{
	ISP_WB_Q_INFO_S wb_info;
	memset(&wb_info, 0, sizeof(ISP_WB_Q_INFO_S));
	CVI_AWB_QueryInfo(ch, &wb_info);
	if (wb_value) {
		*wb_value = wb_info.u16ColorTemp;
	}
	return 0;
}

int mmf_set_wb(int ch, uint16_t wb_value)
{
	ISP_WB_ATTR_S stWbAttr;
	CVI_ISP_GetWBAttr(ch, &stWbAttr);
	dump_wb_attr(&stWbAttr);
	stWbAttr.stAuto.u16RefColorTemp = wb_value;
	stWbAttr.stAuto.u16HighColorTemp = wb_value + 500;
	stWbAttr.stAuto.u16LowColorTemp = wb_value - 500;
	printf("==========[%s][%d] set wb_value:%d\n", __func__, __LINE__, wb_value);
	CVI_ISP_SetWBAttr(ch, &stWbAttr);
	CVI_ISP_GetWBAttr(ch, &stWbAttr);
	dump_wb_attr(&stWbAttr);
	return 0;
}

// static void _awb_test(void)
// {
// 	static uint16_t colortemp = 0;
// 	static int up_flag = 1;
// 	static int init = 0;
// 	if (!init) {
// 		mmf_get_wb(0, &colortemp);
// 		printf("==========[%s][%d] get wb_value:%d\n", __func__, __LINE__, colortemp);

// 		mmf_set_wb(0, 1000);
// 		init = 1;
// 	}

// 	if (colortemp >= 9000) {
// 		up_flag = 0;
// 		colortemp = 9000;
// 	} else if (colortemp <= 1000) {
// 		up_flag = 1;
// 		colortemp = 1000;
// 	}

// 	if (up_flag) {
// 		colortemp += 1000;
// 	} else {
// 		colortemp -= 1000;
// 	}

// 	// mmf_set_wb(0, colortemp);

// 	uint16_t wb_value2 = 0;
// 	mmf_get_wb(0, &wb_value2);
// 	printf("==========[%s][%d] get wb_value2:%d\n", __func__, __LINE__, wb_value2);
// }

static int _test_vio(void)
{
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	int img_w = 480, img_h = 640, fit = 2, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888

	// mmf_vb_config_of_vo(ALIGN(img_w, DEFAULT_ALIGN) * img_h * 3 / 2, 8);
	// mmf_vb_config_of_private(ALIGN(img_w, DEFAULT_ALIGN) * img_h * 3 / 2, 1);

	if (0 != mmf_init()) {
		DEBUG("mmf_init failed!\r\n");
		return -1;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	// mmf_set_vi_hmirror(vi_ch, true);
	// mmf_set_vi_vflip(vi_ch, true);
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_vi_deinit();
		mmf_deinit();
		return -1;
	}

	usleep(1000 * 1000);
	uint32_t exptime = 0, iso_num = 0;
	mmf_get_exptime_and_iso(0, &exptime, &iso_num);
	printf("==========[%s][%d] get exp:%d iso:%d\n",
						__func__, __LINE__, exptime, iso_num);
	int layer = 0;
	int vo_ch = mmf_get_vo_unused_channel(layer);
	if (0 != mmf_add_vo_channel(layer, vo_ch, 552, 368, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	uint64_t start, start2 = 0;
	void *data;
	int data_size, width, height, format;

	// first snap, do nothing
	if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
		mmf_vi_frame_free(vi_ch);
	}
	while (!exit_flag) {
		int show_img_size = 0;
		if (img_fmt == PIXEL_FORMAT_RGB_888)
			show_img_size = img_w * img_h * 3;
		else
			show_img_size = img_w * img_h * 3 / 2;

		uint8_t *show_img = malloc(show_img_size);
		if (!show_img) {
			DEBUG("Malloc failed!\r\n");
			exit_flag = 1;
		}

		start = _get_time_us();
		if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			DEBUG(">>>>>> pop vi frame %ld us\n", _get_time_us() - start);

			DEBUG("Pop..width:%d height:%d data_size:%d format:%d\r\n", width, height, data_size, format);

			start = _get_time_us();
			if (img_w % DEFAULT_ALIGN != 0) {
				switch (img_fmt) {
					case PIXEL_FORMAT_RGB_888:
					for (int h = 0; h < img_h; h ++) {
						memcpy(show_img + h * img_w * 3, (uint8_t *)data + h * width * 3, img_w * 3);
					}
					break;
					case PIXEL_FORMAT_NV21:
					for (int h = 0; h < img_h * 3 / 2; h ++) {
						memcpy(show_img + h * img_w, (uint8_t *)data + h * width, img_w);
					}
					break;
					default:break;
				}
			} else {
				memcpy(show_img, data, data_size);
			}

			mmf_vi_frame_free(vi_ch);

			if (img_fmt == PIXEL_FORMAT_RGB_888) {
				for (int i = 0; i < img_h; i ++) {
					uint8_t *buff = &show_img[(i * img_w + i) * 3];
					buff[0] = 0xff;
					buff[1] = 0x00;
					buff[2] = 0x00;
				}
				for (int i = 0; i < img_h; i ++) {
					uint8_t *buff = &show_img[(i * img_w + i + img_w - img_h) * 3];
					buff[0] = 0x00;
					buff[1] = 0xff;
					buff[2] = 0x00;
				}
			}
			DEBUG(">>>>>> mmcpy vi frame %ld\n", _get_time_us() - start);

			start = _get_time_us();
			DEBUG("Push..width:%d height:%d data_size:%d format:%d\r\n", img_w, img_h, show_img_size, img_fmt);
			mmf_vo_frame_push(layer, vo_ch, show_img, show_img_size, img_w, img_h, format, fit);
			DEBUG(">>>>>> flush vo frame %ld\n", _get_time_us() - start);

			// static int cnt = 0;
			// if (cnt ++ > 10) {
			// 	save_buff_to_file("480_480.yuv", show_img, show_img_size);
			// 	DEBUG("Save to snapshot.yuv\r\n");
			// 	system("sync");
			// 	exit_flag = 1;
			// }

			DEBUG(">>>>>> flush time %ld ms\n", (_get_time_us() - start2) / 1000);
			start2 = _get_time_us();
			// PAUSE();
		}
		UNUSED(save_buff_to_file);
		free(show_img);
	}

	mmf_del_vo_channel(layer, vo_ch);
	mmf_del_vi_channel(vi_ch);
	mmf_vi_deinit();
	mmf_deinit();
	return 0;
}

static int _test_region(void)
{
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

#if 1
	if (0 != mmf_init()) {
		DEBUG("mmf_init failed!\r\n");
		return -1;
	}

	int img_w = 552, img_h = 368, fit = 0, img_fmt = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	int vo_w = 552, vo_h = 368;
	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int layer = 0;
	int vo_ch = mmf_get_vo_unused_channel(layer);
	if (0 != mmf_add_vo_channel(layer, vo_ch, vo_w, vo_h, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	int rgn_ch = 0, rgn_w = 200, rgn_h = 100, rgn_x = 0, rgn_y = 0;
	rgn_ch = mmf_get_region_unused_channel();
	if (0 != mmf_add_region_channel(rgn_ch, OVERLAY_RGN, CVI_ID_VPSS, 1, vo_ch, rgn_x, rgn_y, rgn_w, rgn_h, PIXEL_FORMAT_ARGB_8888)) {
		DEBUG("mmf_add_region_channel failed!\r\n");
		exit_flag = 1;
	}

	int rgn_ch2 = mmf_get_region_unused_channel();
	if (0 != mmf_add_region_channel(rgn_ch2, OVERLAY_RGN, CVI_ID_VPSS, 0, vi_ch, rgn_x + 100, rgn_y + 100, rgn_w, rgn_h, PIXEL_FORMAT_ARGB_8888)) {
		DEBUG("mmf_add_region_channel failed!\r\n");
		exit_flag = 1;
	}

	struct timeval tv;
	uint64_t start, start2, end;
	void *data;
	int data_size, width, height, format;

	// first snap, do nothing
	if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
		mmf_vo_frame_push(layer, vo_ch, data, data_size, width, height, format, fit);
		mmf_vi_frame_free(vi_ch);
	}

	while (!exit_flag) {
		int show_img_size = 0;
		if (img_fmt == PIXEL_FORMAT_RGB_888)
			show_img_size = img_w * img_h * 3;
		else
			show_img_size = img_w * img_h * 3 / 2;

		uint8_t *show_img = malloc(show_img_size);
		if (!show_img) {
			DEBUG("Malloc failed!\r\n");
			exit_flag = 1;
		}

		gettimeofday(&tv, NULL);
		start = tv.tv_usec + tv.tv_sec * 1000000;
		start2 = start;
		if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			gettimeofday(&tv, NULL);
			end = tv.tv_usec + tv.tv_sec * 1000000;
			DEBUG(">>>>>> pop vi frame %ld\n", end - start);
			start = end;

			DEBUG("Pop..width:%d height:%d data_size:%d format:%d\r\n", width, height, data_size, format);
			if (img_w % DEFAULT_ALIGN != 0) {
				switch (img_fmt) {
					case PIXEL_FORMAT_RGB_888:
					for (int h = 0; h < img_h; h ++) {
						memcpy(show_img + h * img_w * 3, (uint8_t *)data + h * width * 3, img_w * 3);
					}
					break;
					case PIXEL_FORMAT_NV21:
					for (int h = 0; h < img_h * 3 / 2; h ++) {
						memcpy(show_img + h * img_w, (uint8_t *)data + h * width, img_w);
					}
					break;
					default:break;
				}
			} else {
				memcpy(show_img, data, data_size);
			}

			gettimeofday(&tv, NULL);
			end = tv.tv_usec + tv.tv_sec * 1000000;
			DEBUG(">>>>>> mmcpy vi frame %ld\n", end - start);
			start = end;

			mmf_vi_frame_free(vi_ch);

			{
				start = _get_time_us();
				uint8_t *rgn_data = (uint8_t *)_prepare_image(rgn_w, rgn_h, PIXEL_FORMAT_ARGB_8888);
				DEBUG("memcpy rgn_data use: %ld us\n", _get_time_us() - start);

				{
					memset(rgn_data, 0xff, rgn_w * rgn_h * 4);
					uint8_t *new_rgn_data = (uint8_t *)rgn_data;
					for (int h = 0; h < 50; h ++) {
						for (int w = 0; w < 50; w ++) {
							new_rgn_data[rgn_w * h * 4 + w * 4 + 0] = 0x00;
							new_rgn_data[rgn_w * h * 4 + w * 4 + 1] = 0xff;
							new_rgn_data[rgn_w * h * 4 + w * 4 + 2] = 0x00;
							new_rgn_data[rgn_w * h * 4 + w * 4 + 3] = 0x55;
						}
					}
				}

				start = _get_time_us();
				mmf_region_frame_push(rgn_ch, rgn_data, rgn_w * rgn_h * 4);
				DEBUG("_mmf_region_frame_push use: %ld us\n", _get_time_us() - start);


				start = _get_time_us();
				void *data;
				int width, height, format;
				assert(0 == mmf_region_get_canvas(rgn_ch2, &data, &width, &height, &format));
				// memcpy(data, rgn_data, rgn_w * rgn_h * 4);
				memset(data, 0xff, rgn_w * rgn_h * 4);
				uint8_t *new_rgn_data = (uint8_t *)data;
				for (int h = 0; h < height; h ++) {
					for (int w = 0; w < width; w ++) {
						new_rgn_data[width * h * 4 + w * 4 + 0] = 0x00;
						new_rgn_data[width * h * 4 + w * 4 + 1] = 0xff;
						new_rgn_data[width * h * 4 + w * 4 + 2] = 0x00;
						new_rgn_data[width * h * 4 + w * 4 + 3] = 0x55;
					}
				}
				assert(0 == mmf_region_update_canvas(rgn_ch2));
				DEBUG("mmf_region_update_canvas use: %ld us\n", _get_time_us() - start);
				free(rgn_data);
			}
			DEBUG("Push..width:%d height:%d data_size:%d format:%d\r\n", img_w, img_h, show_img_size, img_fmt);
			mmf_vo_frame_push(layer, vo_ch, show_img, show_img_size, img_w, img_h, format, fit);

			gettimeofday(&tv, NULL);
			end = tv.tv_usec + tv.tv_sec * 1000000;
			DEBUG(">>>>>> flush vo frame %ld\n", end - start);

			gettimeofday(&tv, NULL);
			end = tv.tv_usec + tv.tv_sec * 1000000;
			DEBUG(">>>>>> flush time %ld\n", (end - start2) / 1000);
			// PAUSE();
		}
		UNUSED(save_buff_to_file);
		free(show_img);
	}

	mmf_del_region_channel(rgn_ch);
	mmf_del_region_channel(rgn_ch2);
	mmf_del_vo_channel(layer, vo_ch);
	mmf_del_vi_channel(vi_ch);
	mmf_deinit();
	return 0;
#else
	if (0 != mmf_init()) {
		DEBUG("mmf_init failed!\r\n");
		return -1;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int img_w = 552, img_h = 368, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	int vo_w = 552, vo_h = 368;
	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int video_layer = 0;
	int vo_video_ch = mmf_get_vo_unused_channel(video_layer);
	if (0 != mmf_add_vo_channel(video_layer, vo_video_ch, vo_w, vo_h, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	int osd_layer = 1;
	int osd_img_fmt = PIXEL_FORMAT_ARGB_8888;
	int vo_osd_ch = mmf_get_vo_unused_channel(osd_layer);
	if (0 != mmf_add_vo_channel(osd_layer, vo_osd_ch, vo_w, vo_h, osd_img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vo_channel(video_layer, vo_video_ch);
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	uint64_t start, last_loop_us = 0;
	void *data;
	int data_size, width, height, format;

	// first snap, do nothing
	if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
		mmf_vo_frame_push(video_layer, vo_video_ch, data, data_size, width, height, format, fit);
		mmf_vi_frame_free(vi_ch);
	}

	{
		uint8_t *rgn_data = _prepare_image(vo_w, vo_h, PIXEL_FORMAT_ARGB_8888);
		start = _get_time_us();
		mmf_vo_frame_push(osd_layer, vo_osd_ch, rgn_data, vo_w * vo_h * 4, img_w, img_h, osd_img_fmt, fit);
		DEBUG("osd frame push use: %ld us\n", _get_time_us() - start);
		free(rgn_data);
	}
	last_loop_us = _get_time_us();
	while (!exit_flag) {
		int show_img_size = 0;
		if (img_fmt == PIXEL_FORMAT_RGB_888)
			show_img_size = img_w * img_h * 3;
		else
			show_img_size = img_w * img_h * 3 / 2;

		uint8_t *show_img = malloc(show_img_size);
		if (!show_img) {
			DEBUG("Malloc failed!\r\n");
			exit_flag = 1;
		}

		start = _get_time_us();
		if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			DEBUG("vi pop frame\n(width:%d height:%d data_size:%d format:%d) use: %ld us\n", width, height, data_size, format, _get_time_us() - start);

			start = _get_time_us();
			if (img_w % DEFAULT_ALIGN != 0) {
				switch (img_fmt) {
					case PIXEL_FORMAT_RGB_888:
					for (int h = 0; h < img_h; h ++) {
						memcpy(show_img + h * img_w * 3, (uint8_t *)data + h * width * 3, img_w * 3);
					}
					break;
					case PIXEL_FORMAT_NV21:
					for (int h = 0; h < img_h * 3 / 2; h ++) {
						memcpy(show_img + h * img_w, (uint8_t *)data + h * width, img_w);
					}
					break;
					default:break;
				}
			} else {
				memcpy(show_img, data, data_size);
			}
			DEBUG("vi memcpy use: %ld us\n", _get_time_us() - start);

			mmf_vi_frame_free(vi_ch);

			if (img_fmt == PIXEL_FORMAT_RGB_888) {
				for (int i = 0; i < img_h; i ++) {
					uint8_t *buff = &show_img[(i * img_w + i) * 3];
					buff[0] = 0xff;
					buff[1] = 0x00;
					buff[2] = 0x00;
				}
				for (int i = 0; i < img_h; i ++) {
					uint8_t *buff = &show_img[(i * img_w + i + img_w - img_h) * 3];
					buff[0] = 0x00;
					buff[1] = 0xff;
					buff[2] = 0x00;
				}
			}

			{
				uint8_t *rgn_data = _prepare_image(vo_w, vo_h, PIXEL_FORMAT_ARGB_8888);
				start = _get_time_us();
				mmf_vo_frame_push(osd_layer, vo_osd_ch, rgn_data, vo_w * vo_h * 4, img_w, img_h, osd_img_fmt, fit);
				DEBUG("osd frame push use: %ld us\n", _get_time_us() - start);
				free(rgn_data);
			}
			DEBUG("Push..width:%d height:%d data_size:%d format:%d\r\n", img_w, img_h, show_img_size, img_fmt);

			start = _get_time_us();
			if (0 != mmf_vo_frame_push(video_layer, vo_video_ch, show_img, show_img_size, img_w, img_h, format, fit)) {
				DEBUG("mmf_vo_frame_push failed!\r\n");
				exit_flag = 1;
			}
			DEBUG("video frame push use: %ld us\n", _get_time_us() - start);
			// PAUSE();
		}
		UNUSED(save_buff_to_file);
		free(show_img);

		DEBUG("loop use: %ld us\n", _get_time_us() - last_loop_us);
		last_loop_us = _get_time_us();
	}

	mmf_del_vo_channel(osd_layer, vo_osd_ch);
	mmf_del_vo_channel(video_layer, vo_video_ch);
	mmf_del_vi_channel(vi_ch);
	mmf_deinit();
	return 0;
#endif
}

static int _test_venc_jpg(void)
{
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	uint8_t *filebuf = NULL;
	uint32_t filelen;
	(void)filelen;

#if 1
	int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	// int img_w = 640, img_h = 480, fit = 0, img_fmt = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888

	filebuf = _prepare_image(img_w, img_h, img_fmt);
	if (img_fmt == PIXEL_FORMAT_RGB_888)
		filelen = img_w * img_h * 3;
	else
		filelen = img_w * img_h * 3 / 2;
	int show_w = 552, show_h = 368;

	DEBUG("in w:%d h:%d fmt:%d\r\n", img_w, img_h, img_fmt);
	DEBUG("out w:%d h:%d fmt:%d\r\n", show_w, show_h, img_fmt);
#endif

	mmf_init();

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int layer = 0;
	int vo_ch = mmf_get_vo_unused_channel(layer);
	if (0 != mmf_add_vo_channel(layer, vo_ch, show_w, show_h, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	int enc_ch = 0;
	uint64_t start = _get_time_us();
#if 1
	while (!exit_flag) {
		// pop last push frame
		uint8_t *data;
		int data_size;
		start = _get_time_us();
		if (!mmf_enc_jpg_pop(enc_ch, &data, &data_size)) {
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

			// save_buff_to_file("venc_stream.jpg", data, data_size);

			start = _get_time_us();
			if (0 != mmf_enc_jpg_free(enc_ch)) {

				DEBUG("mmf_enc_jpg_free failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
		}

		start = _get_time_us();
		if (0 != mmf_enc_jpg_push(enc_ch, filebuf, img_w, img_h, img_fmt)) {
			DEBUG("mmf_enc_jpg_push failed!\r\n");
		}
		DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
		// break;
	}
#elif 1
	{
		if (0 != mmf_enc_jpg_init(enc_ch, img_w, img_h, img_fmt, 80)) {
			DEBUG("mmf_enc_jpg_init failed!\r\n");
			return -1;
		}
		DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);


		while (!exit_flag) {
		start = _get_time_us();
			if (0 != mmf_enc_jpg_push(enc_ch, filebuf, img_w, img_h, img_fmt)) {
				DEBUG("mmf_enc_jpg_push failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

			uint8_t *data;
			int data_size;
			start = _get_time_us();
			if (0 != mmf_enc_jpg_pop(enc_ch, &data, &data_size)) {
				DEBUG("mmf_enc_jpg_pop failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

			save_buff_to_file("venc_stream.jpg", data, data_size);

			start = _get_time_us();
			if (0 != mmf_enc_jpg_free(enc_ch)) {
				DEBUG("mmf_enc_jpg_free failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
		}
		start = _get_time_us();
		if (0 != mmf_enc_jpg_deinit(enc_ch)) {
			DEBUG("mmf_enc_jpg_deinit failed!\r\n");
		}
		DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
	}
#else
	while (!exit_flag) {
		{
			void *data;
			int data_size, width, height, format;
			if (0 == mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
				DEBUG("Pop..width:%d height:%d data_size:%d format:%d\r\n", width, height, data_size, format);
				mmf_vi_frame_free(vi_ch);
			}
		}

		{
			start = _get_time_us();
			if (0 != mmf_enc_jpg_push(enc_ch, filebuf, img_w, img_h, img_fmt)) {
				DEBUG("mmf_enc_jpg_push failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

			uint8_t *data;
			int data_size;
			start = _get_time_us();
			if (0 != mmf_enc_jpg_pop(enc_ch, &data, &data_size)) {
				DEBUG("mmf_enc_jpg_pop failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

			// save_buff_to_file("venc_stream.jpg", data, data_size);
			start = _get_time_us();DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
			if (0 != mmf_enc_jpg_free(enc_ch)) {
				DEBUG("mmf_enc_jpg_free failed!\r\n");
			}
			DEBUG(">>>>>>>>>>>>>>>>[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
			// break;
		}

		{
			mmf_vo_frame_push(layer, vo_ch, filebuf, filelen, img_w, img_h, img_fmt, fit);
			usleep(33 * 1000);
		}
	}
#endif

	mmf_del_vo_channel(layer, vo_ch);
	mmf_del_vi_channel(vi_ch);
	mmf_deinit();
	return 0;
}


static VIDEO_FRAME_INFO_S *_mmf_alloc_frame(SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat)
{
	VIDEO_FRAME_INFO_S *pstVideoFrame;
	VIDEO_FRAME_S *pstVFrame;
	VB_BLK blk;
	VB_CAL_CONFIG_S stVbCfg;

	pstVideoFrame = (VIDEO_FRAME_INFO_S *)calloc(sizeof(*pstVideoFrame), 1);
	if (pstVideoFrame == NULL) {
		SAMPLE_PRT("Failed to allocate VIDEO_FRAME_INFO_S\n");
		return NULL;
	}

	memset(&stVbCfg, 0, sizeof(stVbCfg));
	VENC_GetPicBufferConfig(stSize.u32Width,
				stSize.u32Height,
				enPixelFormat,
				DATA_BITWIDTH_8,
				COMPRESS_MODE_NONE,
				&stVbCfg);

	pstVFrame = &pstVideoFrame->stVFrame;

	pstVFrame->enCompressMode = COMPRESS_MODE_NONE;
	pstVFrame->enPixelFormat = enPixelFormat;
	pstVFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
	pstVFrame->enColorGamut = COLOR_GAMUT_BT709;
	pstVFrame->u32Width = stSize.u32Width;
	pstVFrame->u32Height = stSize.u32Height;
	pstVFrame->u32TimeRef = 0;
	pstVFrame->u64PTS = 0;
	pstVFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;

	if (pstVFrame->u32Width % VENC_ALIGN_W) {
		SAMPLE_PRT("u32Width is not algined to %d\n", VENC_ALIGN_W);
	}

	blk = CVI_VB_GetBlock(VB_INVALID_POOLID, stVbCfg.u32VBSize);
	if (blk == VB_INVALID_HANDLE) {
		SAMPLE_PRT("Can't acquire vb block\n");
		free(pstVideoFrame);
		return NULL;
	}

	pstVideoFrame->u32PoolId = CVI_VB_Handle2PoolId(blk);
	pstVFrame->u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);
	pstVFrame->u32Stride[0] = stVbCfg.u32MainStride;
	pstVFrame->u32Length[0] = stVbCfg.u32MainYSize;
	pstVFrame->pu8VirAddr[0] = (CVI_U8 *)CVI_SYS_MmapCache(pstVFrame->u64PhyAddr[0], pstVFrame->u32Length[0]);

	if (stVbCfg.plane_num > 1) {
		pstVFrame->u64PhyAddr[1] = ALIGN(pstVFrame->u64PhyAddr[0] + stVbCfg.u32MainYSize, stVbCfg.u16AddrAlign);
		pstVFrame->u32Stride[1] = stVbCfg.u32CStride;
		pstVFrame->u32Length[1] = stVbCfg.u32MainCSize;
		pstVFrame->pu8VirAddr[1] = (CVI_U8 *)CVI_SYS_MmapCache(pstVFrame->u64PhyAddr[1], pstVFrame->u32Length[1]);
	}

	if (stVbCfg.plane_num > 2) {
		pstVFrame->u64PhyAddr[2] = ALIGN(pstVFrame->u64PhyAddr[1] + stVbCfg.u32MainCSize, stVbCfg.u16AddrAlign);
		pstVFrame->u32Stride[2] = stVbCfg.u32CStride;
		pstVFrame->u32Length[2] = stVbCfg.u32MainCSize;
		pstVFrame->pu8VirAddr[2] = (CVI_U8 *)CVI_SYS_MmapCache(pstVFrame->u64PhyAddr[2], pstVFrame->u32Length[2]);
	}

	// CVI_VENC_TRACE("phy addr(%#llx, %#llx, %#llx), Size %x\n", (long long)pstVFrame->u64PhyAddr[0]
	// 	, (long long)pstVFrame->u64PhyAddr[1], (long long)pstVFrame->u64PhyAddr[2], stVbCfg.u32VBSize);
	// CVI_VENC_TRACE("vir addr(%p, %p, %p), Size %x\n", pstVFrame->pu8VirAddr[0]
	// 	, pstVFrame->pu8VirAddr[1], pstVFrame->pu8VirAddr[2], stVbCfg.u32MainSize);

	return pstVideoFrame;
}

static CVI_S32 _mmf_free_frame(VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VIDEO_FRAME_S *pstVFrame = &pstVideoFrame->stVFrame;
	VB_BLK blk;

	if (pstVFrame->pu8VirAddr[0])
		CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[0], pstVFrame->u32Length[0]);
	if (pstVFrame->pu8VirAddr[1])
		CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[1], pstVFrame->u32Length[1]);
	if (pstVFrame->pu8VirAddr[2])
		CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[2], pstVFrame->u32Length[2]);

	blk = CVI_VB_PhysAddr2Handle(pstVFrame->u64PhyAddr[0]);
	if (blk != VB_INVALID_HANDLE) {
		CVI_VB_ReleaseBlock(blk);
	}

	free(pstVideoFrame);

	return CVI_SUCCESS;
}


// static void _mmf_dump_venc_h265_vui(VENC_H265_VUI_S *venc_h265_vui)
// {
//     printf("venc_h265_vui->stVuiAspectRatio.aspect_ratio_info_present_flag = %d\n", venc_h265_vui->stVuiAspectRatio.aspect_ratio_info_present_flag);
//     printf("venc_h265_vui->stVuiAspectRatio.aspect_ratio_idc = %d\n", venc_h265_vui->stVuiAspectRatio.aspect_ratio_idc);
//     printf("venc_h265_vui->stVuiAspectRatio.overscan_info_present_flag = %d\n", venc_h265_vui->stVuiAspectRatio.overscan_info_present_flag);
//     printf("venc_h265_vui->stVuiAspectRatio.overscan_appropriate_flag = %d\n", venc_h265_vui->stVuiAspectRatio.overscan_appropriate_flag);
//     printf("venc_h265_vui->stVuiAspectRatio.sar_width = %d\n", venc_h265_vui->stVuiAspectRatio.sar_width);
//     printf("venc_h265_vui->stVuiAspectRatio.sar_height = %d\n", venc_h265_vui->stVuiAspectRatio.sar_height);

//     printf("venc_h265_vui->stVuiTimeInfo.timing_info_present_flag = %d\n", venc_h265_vui->stVuiTimeInfo.timing_info_present_flag);
//     printf("venc_h265_vui->stVuiTimeInfo.num_units_in_tick = %d\n", venc_h265_vui->stVuiTimeInfo.num_units_in_tick);
//     printf("venc_h265_vui->stVuiTimeInfo.time_scale = %d\n", venc_h265_vui->stVuiTimeInfo.time_scale);
//     printf("venc_h265_vui->stVuiTimeInfo.num_ticks_poc_diff_one_minus1 = %d\n", venc_h265_vui->stVuiTimeInfo.num_ticks_poc_diff_one_minus1);

//     printf("venc_h265_vui->stVuiVideoSignal.video_signal_type_present_flag = %d\n", venc_h265_vui->stVuiVideoSignal.video_signal_type_present_flag);
//     printf("venc_h265_vui->stVuiVideoSignal.video_format = %d\n", venc_h265_vui->stVuiVideoSignal.video_format);
//     printf("venc_h265_vui->stVuiVideoSignal.video_full_range_flag = %d\n", venc_h265_vui->stVuiVideoSignal.video_full_range_flag);
//     printf("venc_h265_vui->stVuiVideoSignal.colour_description_present_flag = %d\n", venc_h265_vui->stVuiVideoSignal.colour_description_present_flag);
//     printf("venc_h265_vui->stVuiVideoSignal.colour_primaries = %d\n", venc_h265_vui->stVuiVideoSignal.colour_primaries);
//     printf("venc_h265_vui->stVuiVideoSignal.transfer_characteristics = %d\n", venc_h265_vui->stVuiVideoSignal.transfer_characteristics);
//     printf("venc_h265_vui->stVuiVideoSignal.matrix_coefficients = %d\n", venc_h265_vui->stVuiVideoSignal.matrix_coefficients);

//     printf("venc_h265_vui->stVuiBitstreamRestric.bitstream_restriction_flag = %d\n", venc_h265_vui->stVuiBitstreamRestric.bitstream_restriction_flag);
// }

// static void _mmf_dump_venc_rc_param(VENC_RC_PARAM_S *venc_rc_param)
// {
//     printf("venc_rc_param->u32ThrdI[%ld] = [", sizeof(venc_rc_param->u32ThrdI) / sizeof(CVI_U32));
//     for (size_t i = 0; i < sizeof(venc_rc_param->u32ThrdI) / sizeof(CVI_U32); i ++) {
//         printf("%d, ", venc_rc_param->u32ThrdI[i]);
//     }
//     printf("]\n");

//     printf("venc_rc_param->u32ThrdP[%ld] = [", sizeof(venc_rc_param->u32ThrdP) / sizeof(CVI_U32));
//     for (size_t i = 0; i < sizeof(venc_rc_param->u32ThrdP) / sizeof(CVI_U32); i ++) {
//         printf("%d, ", venc_rc_param->u32ThrdP[i]);
//     }
//     printf("]\n");

//     printf("venc_rc_param->u32ThrdB[%ld] = [", sizeof(venc_rc_param->u32ThrdB) / sizeof(CVI_U32));
//     for (size_t i = 0; i < sizeof(venc_rc_param->u32ThrdB) / sizeof(CVI_U32); i ++) {
//         printf("%d, ", venc_rc_param->u32ThrdB[i]);
//     }
//     printf("]\n");

//     printf("venc_rc_param->u32DirectionThrd:%d\n", venc_rc_param->u32DirectionThrd);
//     printf("venc_rc_param->u32RowQpDelta:%d\n", venc_rc_param->u32RowQpDelta);
//     printf("venc_rc_param->s32FirstFrameStartQp:%d\n", venc_rc_param->s32FirstFrameStartQp);
//     printf("venc_rc_param->s32InitialDelay:%d\n", venc_rc_param->s32InitialDelay);
//     printf("venc_rc_param->u32ThrdLv:%d\n", venc_rc_param->u32ThrdLv);
//     printf("venc_rc_param->bBgEnhanceEn:%d\n", venc_rc_param->bBgEnhanceEn);
//     printf("venc_rc_param->s32BgDeltaQp:%d\n", venc_rc_param->s32BgDeltaQp);
//     printf("venc_rc_param->u32RowQpDelta:%d\n", venc_rc_param->u32RowQpDelta);

//     printf("venc_rc_param->stParamH264Cbr.u32MinIprop:%d\n", venc_rc_param->stParamH264Cbr.u32MinIprop);
//     printf("venc_rc_param->stParamH264Cbr.u32MaxIprop:%d\n", venc_rc_param->stParamH264Cbr.u32MaxIprop);
//     printf("venc_rc_param->stParamH264Cbr.u32MaxQp:%d\n", venc_rc_param->stParamH264Cbr.u32MaxQp);
//     printf("venc_rc_param->stParamH264Cbr.u32MinQp:%d\n", venc_rc_param->stParamH264Cbr.u32MinQp);
//     printf("venc_rc_param->stParamH264Cbr.u32MaxIQp:%d\n", venc_rc_param->stParamH264Cbr.u32MaxIQp);
//     printf("venc_rc_param->stParamH264Cbr.u32MinIQp:%d\n", venc_rc_param->stParamH264Cbr.u32MinIQp);
//     printf("venc_rc_param->stParamH264Cbr.s32MaxReEncodeTimes:%d\n", venc_rc_param->stParamH264Cbr.s32MaxReEncodeTimes);
//     printf("venc_rc_param->stParamH264Cbr.bQpMapEn:%d\n", venc_rc_param->stParamH264Cbr.bQpMapEn);

//     printf("venc_rc_param->stParamH265Cbr.u32MinIprop:%d\n", venc_rc_param->stParamH265Cbr.u32MinIprop);
//     printf("venc_rc_param->stParamH265Cbr.u32MaxIprop:%d\n", venc_rc_param->stParamH265Cbr.u32MaxIprop);
//     printf("venc_rc_param->stParamH265Cbr.u32MaxQp:%d\n", venc_rc_param->stParamH265Cbr.u32MaxQp);
//     printf("venc_rc_param->stParamH265Cbr.u32MinQp:%d\n", venc_rc_param->stParamH265Cbr.u32MinQp);
//     printf("venc_rc_param->stParamH265Cbr.u32MaxIQp:%d\n", venc_rc_param->stParamH265Cbr.u32MaxIQp);
//     printf("venc_rc_param->stParamH265Cbr.u32MinIQp:%d\n", venc_rc_param->stParamH265Cbr.u32MinIQp);
//     printf("venc_rc_param->stParamH265Cbr.s32MaxReEncodeTimes:%d\n", venc_rc_param->stParamH265Cbr.s32MaxReEncodeTimes);
//     printf("venc_rc_param->stParamH265Cbr.bQpMapEn:%d\n", venc_rc_param->stParamH265Cbr.bQpMapEn);
//     printf("venc_rc_param->stParamH265Cbr.enQpMapMode:%d\n", venc_rc_param->stParamH265Cbr.enQpMapMode);

// }

// static void _mmf_dump_venc_framelost(VENC_FRAMELOST_S *venc_framelost)
// {
//     printf("venc_framelost->bFrmLostOpen:%d\n", venc_framelost->bFrmLostOpen);
//     printf("venc_framelost->u32FrmLostBpsThr:%d\n", venc_framelost->u32FrmLostBpsThr);
//     printf("venc_framelost->enFrmLostMode:%d\n", venc_framelost->enFrmLostMode);
//     printf("venc_framelost->u32EncFrmGaps:%d\n", venc_framelost->u32EncFrmGaps);
// }

// static void _mmf_dump_venc_attr(VENC_ATTR_S *venc_attr)
// {
//     printf("venc_attr->enType = %d\n", venc_attr->enType);
//     printf("venc_attr->u32MaxPicWidth = %d\n", venc_attr->u32MaxPicWidth);
//     printf("venc_attr->u32MaxPicHeight = %d\n", venc_attr->u32MaxPicHeight);
//     printf("venc_attr->u32BufSize = %d\n", venc_attr->u32BufSize);
//     printf("venc_attr->u32Profile = %d\n", venc_attr->u32Profile);
//     printf("venc_attr->bByFrame = %d\n", venc_attr->bByFrame);
//     printf("venc_attr->u32PicWidth = %d\n", venc_attr->u32PicWidth);
//     printf("venc_attr->u32PicHeight = %d\n", venc_attr->u32PicHeight);
//     printf("venc_attr->bSingleCore = %d\n", venc_attr->bSingleCore);
//     printf("venc_attr->bEsBufQueueEn = %d\n", venc_attr->bEsBufQueueEn);
//     printf("venc_attr->bIsoSendFrmEn = %d\n", venc_attr->bIsoSendFrmEn);
// }

// static void _mmf_dump_venc_gop_attr(VENC_GOP_ATTR_S *venc_gop_attr)
// {
//     printf("venc_gop_attr->enGopMode = %d\n", venc_gop_attr->enGopMode);
//     switch (venc_gop_attr->enGopMode) {
//     case VENC_GOPMODE_NORMALP:
//         printf("venc_gop_attr->stNormalP.s32IPQpDelta = %d\n", venc_gop_attr->stNormalP.s32IPQpDelta);
//         break;
//     default:
//         printf("unknown gop mode:%d\n", venc_gop_attr->enGopMode);
//         break;
//     }
// }

// static void _mmf_dump_venc_rc_attr(VENC_RC_ATTR_S *venc_rc_attr)
// {
//     printf("venc_rc_attr->enRcMode = %d\n", venc_rc_attr->enRcMode);
//     switch (venc_rc_attr->enRcMode) {
//     case VENC_RC_MODE_H264CBR:
//         printf("venc_rc_attr->stH264Cbr.u32Gop = %d\n", venc_rc_attr->stH264Cbr.u32Gop);
//         printf("venc_rc_attr->stH264Cbr.u32StatTime = %d\n", venc_rc_attr->stH264Cbr.u32StatTime);
//         printf("venc_rc_attr->stH264Cbr.u32SrcFrameRate = %d\n", venc_rc_attr->stH264Cbr.u32SrcFrameRate);
//         printf("venc_rc_attr->stH264Cbr.fr32DstFrameRate = %d\n", venc_rc_attr->stH264Cbr.fr32DstFrameRate);
//         printf("venc_rc_attr->stH264Cbr.u32BitRate = %d\n", venc_rc_attr->stH264Cbr.u32BitRate);
//         printf("venc_rc_attr->stH264Cbr.bVariFpsEn = %d\n", venc_rc_attr->stH264Cbr.bVariFpsEn);
//         break;
//     case VENC_RC_MODE_MJPEGCBR:
//         printf("venc_rc_attr->stMjpegCbr.u32StatTime = %d\n", venc_rc_attr->stMjpegCbr.u32StatTime);
//         printf("venc_rc_attr->stMjpegCbr.u32SrcFrameRate = %d\n", venc_rc_attr->stMjpegCbr.u32SrcFrameRate);
//         printf("venc_rc_attr->stMjpegCbr.fr32DstFrameRate = %d\n", venc_rc_attr->stMjpegCbr.fr32DstFrameRate);
//         printf("venc_rc_attr->stMjpegCbr.u32BitRate = %d\n", venc_rc_attr->stMjpegCbr.u32BitRate);
//         printf("venc_rc_attr->stMjpegCbr.bVariFpsEn = %d\n", venc_rc_attr->stMjpegCbr.bVariFpsEn);
//         break;
//     case VENC_RC_MODE_H265CBR:
//         printf("venc_rc_attr->stH265Cbr.u32Gop = %d\n", venc_rc_attr->stH265Cbr.u32Gop);
//         printf("venc_rc_attr->stH265Cbr.u32StatTime = %d\n", venc_rc_attr->stH265Cbr.u32StatTime);
//         printf("venc_rc_attr->stH265Cbr.u32SrcFrameRate = %d\n", venc_rc_attr->stH265Cbr.u32SrcFrameRate);
//         printf("venc_rc_attr->stH265Cbr.fr32DstFrameRate = %d\n", venc_rc_attr->stH265Cbr.fr32DstFrameRate);
//         printf("venc_rc_attr->stH265Cbr.u32BitRate = %d\n", venc_rc_attr->stH265Cbr.u32BitRate);
//         printf("venc_rc_attr->stH265Cbr.bVariFpsEn = %d\n", venc_rc_attr->stH265Cbr.bVariFpsEn);
//         break;
//     default:
//         printf("unknown rc mode:%d\n", venc_rc_attr->enRcMode);
//         break;
//     }
// }

// static void _mmf_dump_venc_ch_attr(VENC_CHN_ATTR_S *venc_chn_attr)
// {
//     _mmf_dump_venc_attr(&venc_chn_attr->stVencAttr);
//     _mmf_dump_venc_gop_attr(&venc_chn_attr->stGopAttr);
//     _mmf_dump_venc_rc_attr(&venc_chn_attr->stRcAttr);
// }

static int _test_venc_h265(void)
{
	uint8_t *filebuf = NULL;
	uint32_t filelen;
	(void)filelen;
#if 1
	int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	// int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	(void)fit;
	filebuf = _prepare_image(img_w, img_h, img_fmt);
	if (img_fmt == PIXEL_FORMAT_RGB_888)
		filelen = img_w * img_h * 3;
	else
		filelen = img_w * img_h * 3 / 2;
	int show_w = 552, show_h = 368;

	DEBUG("in w:%d h:%d fmt:%d\r\n", img_w, img_h, img_fmt);
	DEBUG("out w:%d h:%d fmt:%d\r\n", show_w, show_h, img_fmt);
#endif

	mmf_init();

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int ch = 0;
	if (mmf_enc_h265_init(ch, img_w, img_h)) {
		printf("mmf_enc_h265_init failed\n");
		return -1;
	}

#if 0
	while (!exit_flag) {
		mmf_h265_stream_t stream;
		if (!mmf_enc_h265_pop(ch, &stream)) {
			for (int i = 0; i < stream.count; i ++) {
				printf("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
			}

			if (mmf_enc_h265_free(ch)) {
				printf("mmf_enc_h265_free failed\n");
				goto _exit;
			}
		}

		if (mmf_enc_h265_push(ch, filebuf, img_w, img_h, img_fmt)) {
			printf("mmf_enc_h265_push failed\n");
			goto _exit;
		}
	}
#else
	while (!exit_flag) {
		if (mmf_enc_h265_push(ch, filebuf, img_w, img_h, img_fmt)) {
			printf("mmf_enc_h265_push failed\n");
			goto _exit;
		}

		mmf_h265_stream_t stream;
		if (mmf_enc_h265_pop(ch, &stream)) {
			printf("mmf_enc_h265_pull failed\n");
			goto _exit;
		}

		{
			for (int i = 0; i < stream.count; i ++) {
				printf("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
			}

			// static FILE *fp = NULL;
			// static int file_num = 0;
			// static int file_duration = 100;
			// if (fp == NULL) {
			// 	printf("open file\n");
			// 	fp = fopen("venc_stream.h265", "wb");
			// 	if (fp == NULL) {
			// 		printf("open file failed\n");
			// 		goto _exit;
			// 	}
			// }

			// for (int i = 0; i < stream.count; i++) {
			// 	fwrite(stream.data[i], stream.data_size[i], 1, fp);
			// }

			// if (++ file_num >= file_duration) {
			// 	printf("close file\n");
			// 	fclose(fp);
			// 	fp = NULL;
			// 	file_num = 0;
			// 	exit_flag = 1;
			// }
		}

		if (mmf_enc_h265_free(ch)) {
			printf("mmf_enc_h265_free failed\n");
			goto _exit;
		}
	}
#endif

	if (mmf_enc_h265_deinit(ch)) {
		printf("mmf_enc_h265_deinit failed\n");
		return -1;
	}
_exit:
	mmf_deinit();
	return 0;
}

static int _test_vi_venc_h265(void)
{
	uint8_t *filebuf = NULL;
	uint32_t filelen;
	(void)filebuf;
	(void)filelen;
#if 1
	int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	// int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	(void)fit;
	filebuf = _prepare_image(img_w, img_h, img_fmt);
	if (img_fmt == PIXEL_FORMAT_RGB_888)
		filelen = img_w * img_h * 3;
	else
		filelen = img_w * img_h * 3 / 2;
	int show_w = 552, show_h = 368;

	DEBUG("in w:%d h:%d fmt:%d\r\n", img_w, img_h, img_fmt);
	DEBUG("out w:%d h:%d fmt:%d\r\n", show_w, show_h, img_fmt);
#endif

	mmf_init();

	int ch = 0;
	if (mmf_enc_h265_init(ch, img_w, img_h)) {
		printf("mmf_enc_h265_init failed\n");
		return -1;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	uint64_t start = _get_time_us();
	uint64_t last_loop_us = start;
	while (!exit_flag) {
		void *data;
		int data_size, width, height, format;

		start = _get_time_us();
		if (mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			continue;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_enc_h265_push(ch, data, img_w, img_h, img_fmt)) {
			printf("mmf_enc_h265_push failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_h265_stream_t stream;
		if (mmf_enc_h265_pop(ch, &stream)) {
			printf("mmf_enc_h265_pull failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		{
			for (int i = 0; i < stream.count; i ++) {
				printf("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
			}

			static FILE *fp = NULL;
			static int file_num = 0;
			static int file_duration = -1;
			if (fp == NULL) {
				printf("open file\n");
				fp = fopen("venc_stream.h265", "wb");
				if (fp == NULL) {
					printf("open file failed\n");
					goto _exit;
				}
			}

			for (int i = 0; i < stream.count; i++) {
				fwrite(stream.data[i], stream.data_size[i], 1, fp);
			}

			if (file_duration != -1 && ++ file_num >= file_duration) {
				printf("close file\n");
				fclose(fp);
				fp = NULL;
				file_num = 0;
				exit_flag = 1;
			} else if (exit_flag)  {
				printf("close file\n");
				fclose(fp);
				fp = NULL;
			}
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_enc_h265_free(ch)) {
			printf("mmf_enc_h265_free failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_vi_frame_free(vi_ch);
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		DEBUG("use %ld us\r\n", _get_time_us() - last_loop_us);
		last_loop_us = _get_time_us();
	}

	if (mmf_enc_h265_deinit(ch)) {
		printf("mmf_enc_h265_deinit failed\n");
		return -1;
	}
_exit:
	mmf_deinit();
	return 0;
}

static int _test_rtsp_h264(void)
{
	printf("Not support!\r\n");
    while(!exit_flag){
        sleep(1);
    }
	return 0;
}


static int _test_vi_venc_h264(void)
{
	uint8_t *filebuf = NULL;
	uint32_t filelen;
	(void)filelen;
#if 1
	int img_w = 1280, img_h = 720, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	// int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	// int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	(void)fit;
	filebuf = _prepare_image(img_w, img_h, img_fmt);
	if (img_fmt == PIXEL_FORMAT_RGB_888)
		filelen = img_w * img_h * 3;
	else
		filelen = img_w * img_h * 3 / 2;
	int show_w = 552, show_h = 368;

	DEBUG("in w:%d h:%d fmt:%d\r\n", img_w, img_h, img_fmt);
	DEBUG("out w:%d h:%d fmt:%d\r\n", show_w, show_h, img_fmt);
#endif

	mmf_init();

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

#if 1
	{
		CVI_S32 s32Ret = CVI_SUCCESS;
		int ch = 0, w = img_w, h = img_h;
		uint64_t start = _get_time_us();
		{
			if (img_fmt != PIXEL_FORMAT_NV21) {
				printf("Only support PIXEL_FORMAT_NV21!\r\n");
				return -1;
			}
			VENC_CHN_ATTR_S stVencChnAttr;
			memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
			stVencChnAttr.stVencAttr.enType = PT_H265;
			stVencChnAttr.stVencAttr.u32MaxPicWidth = w;
			stVencChnAttr.stVencAttr.u32MaxPicHeight = h;
			stVencChnAttr.stVencAttr.u32BufSize = 1024 * 1024;	// 1024Kb
			stVencChnAttr.stVencAttr.bByFrame = 1;
			stVencChnAttr.stVencAttr.u32PicWidth = w;
			stVencChnAttr.stVencAttr.u32PicHeight = h;
			stVencChnAttr.stVencAttr.bEsBufQueueEn = CVI_TRUE;
			stVencChnAttr.stVencAttr.bIsoSendFrmEn = CVI_TRUE;
			stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
			stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;
			stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
			stVencChnAttr.stRcAttr.stH265Cbr.u32Gop = 50;
			stVencChnAttr.stRcAttr.stH265Cbr.u32StatTime = 2;
			stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRate = 30;
			stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate = 30;
			stVencChnAttr.stRcAttr.stH265Cbr.u32BitRate = 3000;
			stVencChnAttr.stRcAttr.stH265Cbr.bVariFpsEn = 0;
			s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VENC_CreateChn [%d] failed with %d\n", ch, s32Ret);
				return s32Ret;
			}

			VENC_RECV_PIC_PARAM_S stRecvParam;
			stRecvParam.s32RecvPicNum = -1;
			s32Ret = CVI_VENC_StartRecvFrame(ch, &stRecvParam);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VENC_StartRecvPic failed with %d\n", s32Ret);
				return CVI_FAILURE;
			}

			{
				VENC_H265_TRANS_S h265Trans = {0};
				s32Ret = CVI_VENC_GetH265Trans(ch, &h265Trans);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_GetH265Trans failed with %d\n", s32Ret);
					return s32Ret;
				}
				h265Trans.cb_qp_offset = 0;
				h265Trans.cr_qp_offset = 0;
				s32Ret = CVI_VENC_SetH265Trans(ch, &h265Trans);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_SetH265Trans failed with %d\n", s32Ret);
					return s32Ret;
				}
			}

			{
				VENC_H265_VUI_S h265Vui = {0};
				s32Ret = CVI_VENC_GetH265Vui(ch, &h265Vui);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_GetH265Vui failed with %d\n", s32Ret);
					return s32Ret;
				}

				h265Vui.stVuiAspectRatio.aspect_ratio_info_present_flag = 0;
				h265Vui.stVuiAspectRatio.aspect_ratio_idc = 1;
				h265Vui.stVuiAspectRatio.overscan_info_present_flag = 0;
				h265Vui.stVuiAspectRatio.overscan_appropriate_flag = 0;
				h265Vui.stVuiAspectRatio.sar_width = 1;
				h265Vui.stVuiAspectRatio.sar_height = 1;
				h265Vui.stVuiTimeInfo.timing_info_present_flag = 1;
				h265Vui.stVuiTimeInfo.num_units_in_tick = 1;
				h265Vui.stVuiTimeInfo.time_scale = 30;
				h265Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 = 1;
				h265Vui.stVuiVideoSignal.video_signal_type_present_flag = 0;
				h265Vui.stVuiVideoSignal.video_format = 5;
				h265Vui.stVuiVideoSignal.video_full_range_flag = 0;
				h265Vui.stVuiVideoSignal.colour_description_present_flag = 0;
				h265Vui.stVuiVideoSignal.colour_primaries = 2;
				h265Vui.stVuiVideoSignal.transfer_characteristics = 2;
				h265Vui.stVuiVideoSignal.matrix_coefficients = 2;
				h265Vui.stVuiBitstreamRestric.bitstream_restriction_flag = 0;

				// _mmf_dump_venc_h265_vui(&h265Vui);

				s32Ret = CVI_VENC_SetH265Vui(ch, &h265Vui);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_SetH265Vui failed with %d\n", s32Ret);
					return s32Ret;
				}
			}

			// rate control
			{
				VENC_RC_PARAM_S stRcParam;
				s32Ret = CVI_VENC_GetRcParam(ch, &stRcParam);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_GetRcParam failed with %d\n", s32Ret);
					return s32Ret;
				}
				stRcParam.s32FirstFrameStartQp = 35;
				stRcParam.stParamH265Cbr.u32MinIprop = 1;
				stRcParam.stParamH265Cbr.u32MaxIprop = 10;
				stRcParam.stParamH265Cbr.u32MaxQp = 51;
				stRcParam.stParamH265Cbr.u32MinQp = 20;
				stRcParam.stParamH265Cbr.u32MaxIQp = 51;
				stRcParam.stParamH265Cbr.u32MinIQp = 20;

				// _mmf_dump_venc_rc_param(&stRcParam);

				s32Ret = CVI_VENC_SetRcParam(ch, &stRcParam);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_SetRcParam failed with %d\n", s32Ret);
					return s32Ret;
				}
			}

			// frame lost set
			{
				VENC_FRAMELOST_S stFL;
				s32Ret = CVI_VENC_GetFrameLostStrategy(ch, &stFL);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_GetFrameLostStrategy failed with %d\n", s32Ret);
					return s32Ret;
				}
				stFL.enFrmLostMode = FRMLOST_PSKIP;

				// _mmf_dump_venc_framelost(&stFL);

				s32Ret = CVI_VENC_SetFrameLostStrategy(ch, &stFL);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_SetFrameLostStrategy failed with %d\n", s32Ret);
					return s32Ret;
				}
			}
		}
		printf("==============================[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

		while (!exit_flag) {
			start = _get_time_us();
			{
				SIZE_S stSize = {(CVI_U32)w, (CVI_U32)h};
				PIXEL_FORMAT_E format = (PIXEL_FORMAT_E)img_fmt;
				VIDEO_FRAME_INFO_S* frame = _mmf_alloc_frame(stSize, (PIXEL_FORMAT_E)format);
				if (!frame) {
					printf("Alloc frame failed!\r\n");
					return -1;
				}
				uint64_t start2 = _get_time_us();
				switch (format) {
					case PIXEL_FORMAT_NV21:
					{
						memcpy(frame->stVFrame.pu8VirAddr[0], filebuf, w * h);
						memcpy(frame->stVFrame.pu8VirAddr[1], filebuf + w * h, w * h / 2);
					}
					break;
					default: return -1;
				}
				printf("==============================[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start2);

				s32Ret = CVI_VENC_SendFrame(ch, frame, 1000);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_GetStream failed with %#x\n", s32Ret);
					return s32Ret;
				}

				_mmf_free_frame(frame);
			}
			printf("==============================[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);

			start = _get_time_us();
			{
				int fd = CVI_VENC_GetFd(ch);
				if (fd < 0) {
					printf("CVI_VENC_GetFd failed with %d\n", fd);
					goto _exit;
				}

				fd_set readFds;
				struct timeval timeoutVal;

				FD_ZERO(&readFds);
				FD_SET(fd, &readFds);
				timeoutVal.tv_sec = 0;
				timeoutVal.tv_usec = 80*1000;
				s32Ret = select(fd + 1, &readFds, NULL, NULL, &timeoutVal);
				if (s32Ret < 0) {
				if (errno == EINTR) {
					printf("VencChn(%d) select failed!\n", ch);
					goto _exit;
				}
				} else if (s32Ret == 0) {
					printf("VencChn(%d) select timeout!\n", ch);
					goto _exit;
				}

				VENC_STREAM_S frame = {0};
				frame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 8);
				if (!frame.pstPack) {
					printf("malloc failed!\r\n");
					return -1;
				}

				// ISP_EXP_INFO_S stExpInfo;
				// memset(&stExpInfo, 0, sizeof(stExpInfo));
				// CVI_ISP_QueryExposureInfo(0, &stExpInfo);
				// CVI_S32 timeout = (1000 * 2) / (stExpInfo.u32Fps / 100); //u32Fps = fps * 100
				CVI_S32 timeout = 100;
				s32Ret = CVI_VENC_GetStream(ch, &frame, timeout);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_GetStream failed with %#x\n", s32Ret);
					free(frame.pstPack);
					return s32Ret;
				}

				printf("frame.u32PackCount = %d frame.pstPack[0].u32Len:%d\n", frame.u32PackCount, frame.u32PackCount ? frame.pstPack[0].u32Len : 0);
				if ((1 == frame.u32PackCount) && (frame.pstPack[0].u32Len > 512 * 1024)) {
					printf("size is over 512K\n");
				} else {
					printf("get new frame\n");
					for (CVI_U32 i = 0; i < frame.u32PackCount; i++) {
						printf("[%d] frame.pstPack.pu8Addr:%p frame.pstPac.u32Len:%d dataPtr:%p dataLen:%d\n",
								i, frame.pstPack[i].pu8Addr, frame.pstPack[i].u32Len, frame.pstPack[i].pu8Addr -  frame.pstPack[i].u32Offset, frame.pstPack[i].u32Len - frame.pstPack[i].u32Offset);
					}
					{
						// check file exit
						static FILE *fp = NULL;
						static int file_num = 0;
						static int file_duration = 100;
						{
							if (fp == NULL) {
								fp = fopen("venc_stream.h265", "wb");
								if (fp == NULL) {
									printf("open file failed\n");
									goto _release_venc;
								}
							}

							for (CVI_U32 i = 0; i < frame.u32PackCount; i++) {
								fwrite(frame.pstPack[i].pu8Addr + frame.pstPack[i].u32Offset, frame.pstPack[i].u32Len - frame.pstPack[i].u32Offset, 1, fp);
							}

							if (++ file_num >= file_duration) {
								printf("close file\n");
								fclose(fp);
								fp = NULL;
								file_num = 0;
								goto _release_venc;
							}
						}
					}
					// save_buff_to_file("venc_stream.jpg",
					// 	frame.pstPack[0].pu8Addr, frame.pstPack[0].u32Len);
				}

				free(frame.pstPack);
				s32Ret = CVI_VENC_ReleaseStream(ch, &frame);
				if (s32Ret != CVI_SUCCESS) {
					printf("CVI_VENC_ReleaseStream failed with %#x\n", s32Ret);
					return s32Ret;
				}
			}
			printf("==============================[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
		}
_release_venc:
		start = _get_time_us();
		{
			s32Ret = CVI_VENC_StopRecvFrame(ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VENC_StopRecvPic failed with %d\n", s32Ret);
			}

			s32Ret = CVI_VENC_ResetChn(ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VENC_ResetChn vechn[%d] failed with %#x!\n",
						ch, s32Ret);
			}

			s32Ret = CVI_VENC_DestroyChn(ch);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VENC_DestroyChn [%d] failed with %d\n", ch, s32Ret);
			}
		}
		printf("==============================[%s][%d] use %ld us\r\n", __func__, __LINE__, _get_time_us() - start);
	}
#endif
_exit:
	mmf_deinit();
	return 0;
}

extern int rtsp_server_init(char *ip, int port);
extern int rtsp_server_deinit(void);
extern int rtsp_server_start(void);

static inline const uint8_t* search_start_code(const uint8_t* ptr, const uint8_t* end)
{
    for(const uint8_t *p = ptr; p + 3 < end; p++)
    {
        if(0x00 == p[0] && 0x00 == p[1] && (0x01 == p[2] || (0x00==p[2] && 0x01==p[3])))
            return p;
    }
	return end;
}

static void* _rtsp_user_thread(void *args)
{
	(void)args;
    uint8_t *m_ptr = NULL;
    size_t m_capacity = 0;
	const char *file = "test.h265";
	FILE* fp = fopen(file, "rb");
    if(fp) {
		fseek(fp, 0, SEEK_END);
		m_capacity = ftell(fp);
		fseek(fp, 0, SEEK_SET);

        m_ptr = (uint8_t*)malloc(m_capacity);
		fread(m_ptr, 1, m_capacity, fp);
		fclose(fp);
	}

	while (1) {
		const uint8_t* end = m_ptr + m_capacity;
		const uint8_t* nalu = search_start_code(m_ptr, end);
		const uint8_t* p = nalu;
		while (p < end) {
			const unsigned char* pn = search_start_code(p + 4, end);
			size_t bytes = pn - nalu;

			rtsp_send_h265_data((uint8_t *)nalu, bytes);

			nalu = pn;
			p = pn;

			usleep(40 * 1000);
		}
	}

	return NULL;
}

static int _test_rtsp_h265(void)
{
	pthread_t pthread_id;
	pthread_create(&pthread_id, NULL, _rtsp_user_thread, NULL);

	rtsp_server_init(NULL, 8554);
	rtsp_server_start();

	printf("rtsp://%s:%d/live\n", rtsp_get_server_ip(), rtsp_get_server_port());

	while (!exit_flag) {

		usleep(30 * 1000);
	}

	rtsp_server_deinit();
	return 0;
}


static int _test_vi_venc_h265_rtsp(void)
{
	if (0 != rtsp_server_init(NULL, 8554)) {
		printf("rtsp server init\n");
		return 0;
	}

	if (0 != rtsp_server_start()) {
		printf("rtsp server start\n");
		return 0;
	}

	if (0 != mmf_init()) {
		printf("mmf deinit\n");
		return 0;
	}

	int img_w = 2560, img_h = 1440, fit = 0, img_fmt = PIXEL_FORMAT_NV21;
	(void)fit;
	int ch = 0;
	if (mmf_enc_h265_init(ch, img_w, img_h)) {
		printf("mmf_enc_h265_init failed\n");
		return -1;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	printf("rtsp://%s:%d/live\n", rtsp_get_server_ip(), rtsp_get_server_port());

	uint64_t start = _get_time_us();
	uint64_t last_loop_us = start;
	while (!exit_flag) {
		void *data;
		int data_size, width, height, format;

		start = _get_time_us();
		if (mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			continue;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_enc_h265_push(ch, data, img_w, img_h, img_fmt)) {
			printf("mmf_enc_h265_push failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_vi_frame_free(vi_ch);
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_h265_stream_t stream;
		if (mmf_enc_h265_pop(ch, &stream)) {
			printf("mmf_enc_h265_pull failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		{
			int stream_size = 0;
			for (int i = 0; i < stream.count; i ++) {
				printf("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
				stream_size += stream.data_size[i];
			}

			if (stream.count > 1) {
				uint8_t *stream_buffer = (uint8_t *)malloc(stream_size);
				if (stream_buffer) {
					int copy_length = 0;
					for (int i = 0; i < stream.count; i ++) {
						memcpy(stream_buffer + copy_length, stream.data[i], stream.data_size[i]);
						copy_length += stream.data_size[i];
					}
					rtsp_send_h265_data(stream_buffer, copy_length);
					free(stream_buffer);
				} else {
					DEBUG("malloc failed!\r\n");
				}
			} else if (stream.count == 1) {
				rtsp_send_h265_data((uint8_t *)stream.data[0], stream.data_size[0]);
			}
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_enc_h265_free(ch)) {
			printf("mmf_enc_h265_free failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		DEBUG("use %ld us\r\n", _get_time_us() - last_loop_us);
		last_loop_us = _get_time_us();
	}

	if (mmf_enc_h265_deinit(ch)) {
		printf("mmf_enc_h265_deinit failed\n");
		return -1;
	}

	if (0 != rtsp_server_deinit()) {
		printf("rtsp server deinit\n");
		return 0;
	}
_exit:
	if (0 != mmf_deinit()) {
		printf("mmf deinit\n");
	}
	return 0;
}

static int _test_multiple_vi(void)
{
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	int img_w = 2560, img_h = 1440, fit = 2, img_fmt = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	int img_w2 = 640, img_h2 = 480, img_fmt2 = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888

	if (0 != mmf_init()) {
		DEBUG("mmf_init failed!\r\n");
		return -1;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch2 = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch2, img_w2, img_h2, img_fmt2)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	int layer = 0;
	int vo_ch = mmf_get_vo_unused_channel(layer);
	if (0 != mmf_add_vo_channel(layer, vo_ch, 552, 368, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	uint64_t start, start2 = 0;
	void *data, *data2;
	int data_size, width, height, format;
	int data_size2, width2, height2, format2;

	int show_img_size = 0;
	if (img_fmt == PIXEL_FORMAT_RGB_888)
		show_img_size = img_w * img_h * 3;
	else
		show_img_size = img_w * img_h * 3 / 2;

	uint8_t *show_img = malloc(show_img_size);
	if (!show_img) {
		DEBUG("Malloc failed!\r\n");
		exit_flag = 1;
	}
	while (!exit_flag) {
		start = _get_time_us();
		if (mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			continue;
		}

		if (mmf_vi_frame_pop(vi_ch2, &data2, &data_size2, &width2, &height2, &format2)) {
			mmf_vi_frame_free(vi_ch);
			continue;
		}

		DEBUG("[0]Pop..width:%d height:%d data_size:%d format:%d\r\n", width, height, data_size, format);
		DEBUG("[1]Pop..width:%d height:%d data_size:%d format:%d\r\n", width2, height2, data_size2, format2);
		start = _get_time_us();
		if (width % DEFAULT_ALIGN != 0) {
			switch (img_fmt) {
				case PIXEL_FORMAT_RGB_888:
				for (int h = 0; h < img_h; h ++) {
					memcpy(show_img + h * img_w * 3, (uint8_t *)data + h * width * 3, img_w * 3);
				}
				break;
				case PIXEL_FORMAT_NV21:
				for (int h = 0; h < img_h * 3 / 2; h ++) {
					memcpy(show_img + h * img_w, (uint8_t *)data + h * width, img_w);
				}
				break;
				default:break;
			}
		} else {
			memcpy(show_img, data, data_size);
		}

		mmf_vi_frame_free(vi_ch);
		mmf_vi_frame_free(vi_ch2);

		if (img_fmt == PIXEL_FORMAT_RGB_888) {
			for (int i = 0; i < img_h; i ++) {
				uint8_t *buff = &show_img[(i * img_w + i) * 3];
				buff[0] = 0xff;
				buff[1] = 0x00;
				buff[2] = 0x00;
			}
			for (int i = 0; i < img_h; i ++) {
				uint8_t *buff = &show_img[(i * img_w + i + img_w - img_h) * 3];
				buff[0] = 0x00;
				buff[1] = 0xff;
				buff[2] = 0x00;
			}
		}
		DEBUG(">>>>>> mmcpy vi frame %ld\n", _get_time_us() - start);

		start = _get_time_us();
		DEBUG("Push..width:%d height:%d data_size:%d format:%d\r\n", img_w, img_h, show_img_size, img_fmt);
		mmf_vo_frame_push(layer, vo_ch, show_img, show_img_size, img_w, img_h, format, fit);
		DEBUG(">>>>>> flush vo frame %ld\n", _get_time_us() - start);

		DEBUG(">>>>>> flush time %ld ms\n", (_get_time_us() - start2) / 1000);
		start2 = _get_time_us();
	}

	mmf_del_vo_channel(layer, vo_ch);
	mmf_del_vi_channel(vi_ch);
	mmf_del_vi_channel(vi_ch2);
	mmf_deinit();
	return 0;
}

static int _test_vi_region_venc_h265_rtsp(void)
{
	if (0 != rtsp_server_init(NULL, 8554)) {
		printf("rtsp server init\n");
		return 0;
	}

	if (0 != rtsp_server_start()) {
		printf("rtsp server start\n");
		return 0;
	}

	if (0 != mmf_init()) {
		printf("mmf deinit\n");
		return 0;
	}

	// int img_w = 1920, img_h = 1080, fit = 2, img_fmt = PIXEL_FORMAT_NV21;
	// int img_w2 = 1920, img_h2 = 1080, img_fmt2 = PIXEL_FORMAT_NV21;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888
	int img_w = 1280, img_h = 720, fit = 2, img_fmt = PIXEL_FORMAT_NV21;
	int img_w2 = 640, img_h2 = 480, img_fmt2 = PIXEL_FORMAT_RGB_888;	//PIXEL_FORMAT_NV21 or PIXEL_FORMAT_RGB_888

	(void)fit;
	int ch = 0;
	if (mmf_enc_h265_init(ch, img_w, img_h)) {
		printf("mmf_enc_h265_init failed\n");
		return -1;
	}

	if (0 != mmf_vi_init()) {
		DEBUG("mmf_vi_init failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch, img_w, img_h, img_fmt)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_deinit();
		return -1;
	}

	int vi_ch2 = mmf_get_vi_unused_channel();
	if (0 != mmf_add_vi_channel(vi_ch2, img_w2, img_h2, img_fmt2)) {
		DEBUG("mmf_add_vi_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_deinit();
		return -1;
	}

	int layer = 0;
	int vo_ch = mmf_get_vo_unused_channel(layer);
	// if (0 != mmf_add_vo_channel(layer, vo_ch, 552, 368, img_fmt, fit)) {
	if (0 != mmf_add_vo_channel(layer, vo_ch, 832, 480, img_fmt, fit)) {
		DEBUG("mmf_add_vo_channel failed!\r\n");
		mmf_del_vi_channel(vi_ch);
		mmf_del_vi_channel(vi_ch2);
		mmf_deinit();
		return -1;
	}

	// int rgn_ch = 0, rgn_w = 200, rgn_h = 100, rgn_x = 0, rgn_y = 0, rgn_fmt = PIXEL_FORMAT_ARGB_8888;
	// rgn_ch = mmf_get_region_unused_channel();
	// if (0 != mmf_add_region_channel(rgn_ch, OVERLAY_RGN, CVI_ID_VPSS, 0, vi_ch, rgn_x, rgn_y, rgn_w, rgn_h, rgn_fmt)) {
	// 	DEBUG("mmf_add_region_channel failed!\r\n");
	// 	exit_flag = 1;
	// }

	// uint8_t *rgn_test_img = (uint8_t *)_prepare_image(rgn_w, rgn_h, rgn_fmt);
	// if (!rgn_test_img) {
	// 	DEBUG("Malloc failed!\r\n");
	// 	exit_flag = 1;
	// }

	printf("rtsp://%s:%d/live\n", rtsp_get_server_ip(), rtsp_get_server_port());

	uint8_t ipdata[30];
	sprintf(ipdata, "rtsp://%s:%d/live\n", rtsp_get_server_ip(), rtsp_get_server_port());
	int fd = -1;
	remove("/root/rtsp_ip_addr.txt");
    fd = open("/root/rtsp_ip_addr.txt", O_WRONLY | O_CREAT, 0777);
    if (fd <= 2) {
        DEBUG("Open filed, fd = %d\r\n", fd);
    }

    int res = 0;
    if ((res = write(fd, ipdata, 30)) < 0) {
        DEBUG("Write failed");
        close(fd);
    }
    close(fd);


	uint64_t start = _get_time_us();
	uint64_t last_loop_us = start;
	uint8_t frame_count = 0;

	void *data, *data2;
	int data_size, width, height, format;
	int data_size2, width2, height2, format2;

	int show_img_size = 0;
	if (img_fmt2 == PIXEL_FORMAT_RGB_888)
		show_img_size = img_w * img_h * 3;
	else
		show_img_size = img_w * img_h * 3 / 2;

	uint8_t *show_img = malloc(show_img_size);
	if (!show_img) {
		DEBUG("Malloc failed!\r\n");
		exit_flag = 1;
	}
	exit_flag = 0;
	while (!exit_flag) {
		if(frame_count < 100)
			frame_count ++;

		start = _get_time_us();
		mmf_h265_stream_t stream;
		if (!mmf_enc_h265_pop(ch, &stream)) {
			DEBUG("use %ld us\r\n", _get_time_us() - start);

			start = _get_time_us();
			{
				int stream_size = 0;
				for (int i = 0; i < stream.count; i ++) {
					// printf("[%d] stream.data:%p stream.len:%d\n", i, stream.data[i], stream.data_size[i]);
					stream_size += stream.data_size[i];
				}

				if (stream.count > 1) {
					uint8_t *stream_buffer = (uint8_t *)malloc(stream_size);
					if (stream_buffer) {
						int copy_length = 0;
						for (int i = 0; i < stream.count; i ++) {
							memcpy(stream_buffer + copy_length, stream.data[i], stream.data_size[i]);
							copy_length += stream.data_size[i];
						}
						rtsp_send_h265_data(stream_buffer, copy_length);
						free(stream_buffer);
					} else {
						DEBUG("malloc failed!\r\n");
					}
				} else if (stream.count == 1) {
					rtsp_send_h265_data((uint8_t *)stream.data[0], stream.data_size[0]);
				}
			}
			DEBUG("use %ld us\r\n", _get_time_us() - start);

			start = _get_time_us();
			if (mmf_enc_h265_free(ch)) {
				printf("mmf_enc_h265_free failed\n");
				goto _exit;
			}
			DEBUG("use %ld us\r\n", _get_time_us() - start);
		}

		start = _get_time_us();
		if (mmf_vi_frame_pop(vi_ch, &data, &data_size, &width, &height, &format)) {
			continue;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_vi_frame_pop(vi_ch2, &data2, &data_size2, &width2, &height2, &format2)) {
			mmf_vi_frame_free(vi_ch);
			continue;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		if (mmf_enc_h265_push(ch, data, img_w, img_h, img_fmt)) {
			printf("mmf_enc_h265_push failed\n");
			goto _exit;
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);


		start = _get_time_us();
		if (width2 % DEFAULT_ALIGN != 0) {
			switch (img_fmt2) {
				case PIXEL_FORMAT_RGB_888:
				for (int h = 0; h < img_h2; h ++) {
					memcpy(show_img + h * img_w2 * 3, (uint8_t *)data2 + h * width * 3, img_w2 * 3);
				}
				break;
				case PIXEL_FORMAT_NV21:
				for (int h = 0; h < img_h2 * 3 / 2; h ++) {
					memcpy(show_img + h * img_w2, (uint8_t *)data2 + h * width, img_w2);
				}
				break;
				default:break;
			}
		} else {
			memcpy(show_img, data, data_size);
		}
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		if(frame_count == 99){
			save_buff_to_file("rtsp_stream.jpg", data, data_size);
		}

		start = _get_time_us();
		mmf_vi_frame_free(vi_ch);
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		mmf_vi_frame_free(vi_ch2);
		DEBUG("use %ld us\r\n", _get_time_us() - start);

		// start = _get_time_us();
		// if (0 != mmf_region_frame_push(rgn_ch, rgn_test_img, rgn_w * rgn_h * 4)) {
		// 	DEBUG("mmf_region_frame_push failed!\r\n");
		// }
		// DEBUG("use %ld us\r\n", _get_time_us() - start);

		start = _get_time_us();
		DEBUG("Push..width:%d height:%d data_size:%d format:%d\r\n", img_w2, img_h2, show_img_size, img_fmt2);
		mmf_vo_frame_push(layer, vo_ch, show_img, show_img_size, img_w, img_h, img_fmt, fit);
		DEBUG(">>>>>> flush vo frame %ld\n", _get_time_us() - start);

		DEBUG("use %ld us\r\n", _get_time_us() - last_loop_us);
		last_loop_us = _get_time_us();
	}

	// if (0 != mmf_del_region_channel(rgn_ch)) {
	// 	DEBUG("mmf_region_bind failed!\r\n");
	// 	exit_flag = 1;
	// }

	if (mmf_enc_h265_deinit(ch)) {
		printf("mmf_enc_h265_deinit failed\n");
		return -1;
	}

	if (0 != rtsp_server_deinit()) {
		printf("rtsp server deinit\n");
		return 0;
	}
_exit:
	if (0 != mmf_deinit()) {
		printf("mmf deinit\n");
	}
	return 0;
}

int test_pre_init(void)
{
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);
	signal(SIGSEGV, exit_handle);
	signal(SIGKILL, exit_handle);
	signal(SIGFPE, exit_handle);
	signal(SIGILL, exit_handle);
	signal(SIGABRT, exit_handle);
	return 0;
}

int test_vo_only(void)
{
	return _test_vo_only();
}

int test_vi_only(void)
{
	return _test_vi_only();
}

int test_vio(void)
{
	return _test_vio();
}

int test_region(void)
{
	return _test_region();
}

int test_venc_jpg(void)
{
	return _test_venc_jpg();
}

int test_venc_h265(void)
{
	return _test_venc_h265();
}

int test_vi_venc_h265(void)
{
	return _test_vi_venc_h265();
}

int test_rtsp_h264(void)
{
	return _test_rtsp_h264();
}

int test_vi_venc_h264(void)
{
	return _test_vi_venc_h264();
}

int test_rtsp_h265(void)
{
	return _test_rtsp_h265();
}

int test_vi_venc_h265_rtsp(void)
{
	return _test_vi_venc_h265_rtsp();
}

int test_multiple_vi(void)
{
	return _test_multiple_vi();
}

int test_vi_region_venc_h265_rtsp(void)
{
	return _test_vi_region_venc_h265_rtsp();
}
