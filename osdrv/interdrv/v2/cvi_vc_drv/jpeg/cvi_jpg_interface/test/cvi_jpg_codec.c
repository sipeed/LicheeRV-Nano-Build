/**
 * @file cvi_jpg_codec.c
 * @author jinf
 * @brief This module contains the test demo definition for jpeg codec
 * component.
 */
#ifdef VC_DRIVER_TEST
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/io.h>
#include <linux/cvi_vc_drv_ioctl.h>

#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include "cvi_jpg_interface.h"
#include "version.h"
#include "cvi_vc_getopt.h"


/* macro define */
#define OUTPUT_FOR_SCALER 0

// system param
#define PARAM_HELP "-help"
#define PARAM_QUIET "-q"
#define PARAM_NUMBER "-n"
// dcoder and encoder
#define PARAM_MODE_TYPE "-t"
#define PARAM_INPUT_FILE_NAME "-i"
#define PARAM_OUTPUT_FILE_NAME "-o"
#define PARAM_PACKEDFORMAT "-pf"
#define PARAM_CHROMAINTERLEAVE "-ci"
#define PARAM_USEPARTIALMODE "-pm"
#define PARAM_PARTIALBUFNUM "-pn"
#define PARAM_ROTANGLE "-ra"
#define PARAM_MIRDIR "-md"
#define PARAM_ROIENABLE "-roi"
#define PARAM_ROIOFFSETX "-rx"
#define PARAM_ROIOFFSETY "-ry"
#define PARAM_ROIWIDTH "-rw"
#define PARAM_ROIHEIGHT "-rh"
#define PARAM_WIDTH "-w"
#define PARAM_HEIGHT "-h"
#define PARAM_CHROMAFORMAT "-cf"
#define PARAM_VDOWNSAMPLING "-vd"
#define PARAM_HDOWNSAMPLING "-hd"

// ===========================================
// ENTYPE = 1 : read file to alloc BUFFER
// ENTYPE = 2 : read file to alloc CVIFRAMEBUF struct
// ENTYPE = 3 : read file to JPU internal CVIFRAMEBUF
// ===========================================
#define ENC_TYPE 3

/* function define */
static void welcome(void);
static void usage(void);
static int read_jpeg_file(char *name, unsigned char **pSrcBuf);
static int write_yuv_file(char *name, unsigned char *pDstBuf, int suffix);

#if ENC_TYPE == 1
static int read_yuv_file(char *inFileName, unsigned char **pSrcBuf,
			 CVIFrameFormat fmt);
#elif ENC_TYPE == 2
static int read_yuv_file2(char *inFileName, unsigned char *pSrcBuf,
			  CVIFrameFormat fmt);
#elif ENC_TYPE == 3
static int read_yuv_file3(CVIJpgHandle jpgHandle, char *inFileName,
			  CVIEncConfigParam encConfig);
#endif
static int write_jpg_file(char *name, unsigned char *pDstBuf, int suffix);

/* Local Statis Variable */
static int bQuiet;
static int iChromaFormat;
static int iPackedFormat;
static int iChromaInterLeave;
static int iUsePartialMode;
static int iUsePartialNum = 4;
static int iRotAngle;
static int iMirDir;
static int iROIEnable;
static int iROIOffsetX;
static int iROIOffsetY;
static int iROIWidth;
static int iROIHeight;
static int iVDownSampling;
static int iHDownSampling;

/* cvi_jpg_test -t 0/1 -n num -i input_file -o output_file */
int jpeg_main(int argc, char **argv)
{
	int ret = 0;
	int i;

	/* param define */
	int type = 0;
	int width = 0;
	int height = 0;
	int counter = 1;
	char *inFileName = vmalloc(256);
	char *outFileName = vmalloc(256);
	CVIJpgHandle jpgHandle = { 0 };
	//CVIDecConfigParam decConfig;
	CVIEncConfigParam encConfig;
	CVIJpgConfig config;
	/* read source file data */
	unsigned char *srcBuf = NULL;
	CVIFRAMEBUF frame_buffer;
	int readLen = 0;
	// char                writeName[256]={0};

	/* initial local temp var */
	i = 0;
	bQuiet = 0;
	iChromaFormat = 0;
	iPackedFormat = 0;
	iChromaInterLeave = 0;
	iUsePartialMode = 0;
	iUsePartialNum = 4;
	iRotAngle = 0;
	iMirDir = 0;
	iROIEnable = 0;
	iROIOffsetX = 0;
	iROIOffsetY = 0;
	iROIWidth = 0;
	iROIHeight = 0;
	iVDownSampling = 0;
	iHDownSampling = 0;


	//memset(&decConfig, 0, sizeof(CVIDecConfigParam));
	memset(&encConfig, 0, sizeof(CVIEncConfigParam));
	memset(&config, 0, sizeof(CVIJpgConfig));

	if (argc < 2) {
		welcome();
		usage();
		vfree(inFileName);
		vfree(outFileName);
		return 1;
	}

	/* parse the arguments */
	for (i = 1; i < argc; i++) {
		if (0 == strcmp((const char *)argv[i], PARAM_QUIET)) {
			bQuiet = 1;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_PACKEDFORMAT)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <packed format>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iPackedFormat = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i],
				       PARAM_CHROMAINTERLEAVE)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <chroma inter leave>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iChromaInterLeave = (int)atoi(
				(const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i],
				       PARAM_USEPARTIALMODE)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <use partial mode>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iUsePartialMode = (int)atoi(
				(const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_PARTIALBUFNUM)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <partial buf num>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iUsePartialNum = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_ROTANGLE)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <rot angle>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iRotAngle = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_MIRDIR)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <mir dir>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iMirDir = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_ROIENABLE)) {
			iROIEnable = 1;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_ROIOFFSETX)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <roi offset x>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iROIOffsetX = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_ROIOFFSETY)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <roi offset y>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iROIOffsetY = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_ROIWIDTH)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <roi width>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iROIWidth = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_ROIHEIGHT)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <roi height>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iROIHeight = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_CHROMAFORMAT)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <chroma format>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iChromaFormat = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_VDOWNSAMPLING)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <chroma format>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iVDownSampling = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 ==
			   strcmp((const char *)argv[i], PARAM_HDOWNSAMPLING)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <chroma format>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			iHDownSampling = (int)atoi((const char *)argv[i + 1]);
			i++;
		}

		else if (0 == strcmp((const char *)argv[i], PARAM_MODE_TYPE)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <mode type>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			type = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_WIDTH)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <width>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			width = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_HEIGHT)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <height>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			height = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_NUMBER)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <number>!\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			counter = (int)atoi((const char *)argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i],
				       PARAM_INPUT_FILE_NAME)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <input file name>\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			strcpy(inFileName, argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i],
				       PARAM_OUTPUT_FILE_NAME)) {
			if (i + 1 >= argc) {
				pr_err("ERROR: missing <output file name>\n");
				vfree(inFileName);
				vfree(outFileName);
				return 1;
			}
			strcpy(outFileName, argv[i + 1]);
			i++;
		} else if (0 == strcmp((const char *)argv[i], PARAM_HELP)) {
			usage();
			vfree(inFileName);
			vfree(outFileName);
			return 0;
		} else if (0 == strcmp((const char *)argv[i], "-v")) {
			if (argc == 2) {
				pr_err("%s:\n\t%s\n", argv[0], JPEG_VERSION);
				vfree(inFileName);
				vfree(outFileName);
				return 0;
			}
		} else {
			pr_err("ERROR: unrecognized parameters %s\n",
			       (const char *)argv[i]);
			welcome();
			usage();
			vfree(inFileName);
			vfree(outFileName);
			return 1;
		}
	}

	welcome();

	/* initial JPU core */
	ret = CVIJpgInit();
	if (0 != ret) {
		pr_err("\nFailed to CVIJpgInit!!!\n");
		vfree(inFileName);
		vfree(outFileName);
		return ret;
	}

	memset(&frame_buffer, 0, sizeof(CVIFRAMEBUF));

	pr_info("type = %d\n", type);

	if (CVIJPGCOD_DEC == (CVIJpgCodType)type) {
		readLen = read_jpeg_file(inFileName, &srcBuf);
		if (0 == readLen)
			goto FIND_ERROR;
	}

	/* setting decode or encode param */
	config.type = (CVIJpgCodType)type;
	if (CVIJPGCOD_ENC == (CVIJpgCodType)type) {
		config.u.enc.picWidth = (int)width;
		config.u.enc.picHeight = (int)height;
		// [0](4:2:0) [1](4:2:2) [2](2:2:4 4:2:2 rotated) [3](4:4:4)
		// [4](4:0:0)
		config.u.enc.sourceFormat = iChromaFormat;
		// [0](CBCR_SEPARATED) [1](CBCR_INTERLEAVE) [2](CRCB_INTERLEAVE)
		config.u.enc.chromaInterleave = iChromaInterLeave;
		config.u.enc.rotAngle = iRotAngle;
		// [0](no mirror) [1](vertical) [2](horizontal) [3](both)
		config.u.enc.mirDir = iMirDir;
		// [0](PLANAR), [1](YUYV), [2](UYVY), [3](YVYU), [4](VYUY),
		// [5](YUV_444 PACKED), [6](RGB_444 PACKED)
		config.u.enc.packedFormat = iPackedFormat;
#if ENC_TYPE == 1
		config.u.enc.src_type = 0;
#elif ENC_TYPE == 2
		config.u.enc.src_type = 1;
#elif ENC_TYPE == 3
		config.u.enc.src_type = 2;
#endif /* ENC_TYPE */
		memset(config.u.enc.jpgMarkerOrder, 0,
		       sizeof(config.u.enc.jpgMarkerOrder));
		config.u.enc.jpgMarkerOrder[0] = 1; // JPEGE_MARKER_SOI;
		config.u.enc.jpgMarkerOrder[1] =
			10; // JPEGE_MARKER_FRAME_INDEX;
		config.u.enc.jpgMarkerOrder[2] = 12; // JPEGE_MARKER_USER_DATA;
		config.u.enc.jpgMarkerOrder[3] = 7; // JPEGE_MARKER_DRI_OPT;
		config.u.enc.jpgMarkerOrder[4] = 2; // JPEGE_MARKER_DQT;
		config.u.enc.jpgMarkerOrder[5] = 4; // JPEGE_MARKER_DHT;
		config.u.enc.jpgMarkerOrder[6] = 8; // JPEGE_MARKER_SOF0;
		config.u.enc.jpgMarkerOrder[7] = 0;
	} else if (CVIJPGCOD_DEC == (CVIJpgCodType)type) {
		// 2'b00: Normal
		// 2'b10: CbCr interleave (e.g. NV12 in 4:2:0 or NV16 in 4:2:2)
		// 2'b11: CrCb interleave (e.g. NV21 in 4:2:0)
		memset(&(config.u.dec), 0, sizeof(CVIDecConfigParam));
		// [0](PLANAR), [1](YUYV), [2](UYVY), [3](YVYU), [4](VYUY),
		// [5](YUV_444 PACKED), [6](RGB_444 PACKED)
		config.u.dec.dec_buf.packedFormat = iPackedFormat;
		// [0](CBCR_SEPARATED) [1](CBCR_INTERLEAVE) [2](CRCB_INTERLEAVE)
		config.u.dec.dec_buf.chromaInterleave = iChromaInterLeave;
		// ROI param
		config.u.dec.roiEnable = iROIEnable;
		config.u.dec.roiWidth = iROIWidth;
		config.u.dec.roiHeight = iROIHeight;
		config.u.dec.roiOffsetX = iROIOffsetX;
		config.u.dec.roiOffsetY = iROIOffsetY;
		// Frame Partial Mode (DON'T SUPPORT)
		config.u.dec.usePartialMode = 0;
		// Rotation Angle (0, 90, 180, 270)
		config.u.dec.rotAngle = iRotAngle;
		// mirror direction (0-no mirror, 1-vertical, 2-horizontal,
		// 3-both)

		config.u.dec.mirDir = iMirDir;
		config.u.dec.iHorScaleMode = iHDownSampling;
		config.u.dec.iVerScaleMode = iVDownSampling;
		config.u.dec.iDataLen = readLen;
		pr_info("iDataLen = %d\n", config.u.dec.iDataLen);
	} else {
		pr_err("ERROR: unrecognized mode type parameters\n");
		usage();
		vfree(inFileName);
		vfree(outFileName);
		return 1;
	}

	/* Open JPU Devices */
	jpgHandle = CVIJpgOpen(config);
	if (NULL == jpgHandle) {
		pr_err("\nFailed to CVIJpgOpen\n");
		goto FIND_ERROR;
	}

	if (CVIJPGCOD_ENC == (CVIJpgCodType)type) {
#if ENC_TYPE == 1
		readLen = read_yuv_file(inFileName, &srcBuf, 0);
#elif ENC_TYPE == 2
		frame_buffer.width = width;
		frame_buffer.height = height;
		frame_buffer.Format = iChromaFormat;
		frame_buffer.strideY = width;
		frame_buffer.strideC = width;
		int alloclen = width * height;
		if (0 != iPackedFormat) {
			if ((5 == iPackedFormat) || (6 == iPackedFormat))
				alloclen *= 3;
			else
				alloclen *= 2;
		}
		frame_buffer.vbY.virt_addr = (unsigned long)vmalloc(alloclen);
		frame_buffer.vbCb.virt_addr = (unsigned long)vmalloc(alloclen);
		frame_buffer.vbCr.virt_addr = (unsigned long)vmalloc(alloclen);
		srcBuf = (unsigned char *)&frame_buffer;
		readLen = read_yuv_file2(inFileName,
					 (unsigned char *)&frame_buffer, 0);
#elif ENC_TYPE == 3
		memcpy(&encConfig, &(config.u.enc), sizeof(CVIEncConfigParam));
		readLen = read_yuv_file3(jpgHandle, inFileName, encConfig);
		pr_info("readLen = %d\n", readLen);
#else
		fprintf(stderr, "DO NOT SUPPORT ENC_TYPE");
		exit(1);
#endif
		if (0 == readLen)
			goto FIND_ERROR;
#if ENC_TYPE == 1
		readLen = 0;
#elif ENC_TYPE == 2
		readLen = 1;
#elif ENC_TYPE == 3
		readLen = 2;
#endif /* ENC_TYPE */
	}

	/* alloc dst buffer and */
	for (i = 0; i < counter; i++) {
		if (!bQuiet)
			pr_info("=========== COUNT %d ==============\n", i);

		/* flush buffer remain data */
		CVIJpgFlush(jpgHandle);
		/* send jpeg data for decode or encode operator */
		ret = CVIJpgSendFrameData(jpgHandle, (void *)srcBuf, readLen,
					  -1);
		if (0 != ret) {
			pr_err("\nFailed to CVIJpgSendFrameData, ret = %d\n",
			       ret);
			goto FIND_ERROR;
		}

		/* after decode or encode, we get data from jpu */
		if (CVIJPGCOD_DEC == config.type) {
			CVIFRAMEBUF cviFrameBuf;
			memset(&cviFrameBuf, 0, sizeof(CVIFRAMEBUF));
			ret = CVIJpgGetFrameData(jpgHandle,
						 (unsigned char *)&cviFrameBuf,
						 sizeof(CVIFRAMEBUF), NULL);
			if (0 != ret) {
				pr_err("\nFailed to CVIJpgGetFrameData, ret = %d\n",
				       ret);
			}

			// printk("dec width = %d, height = %d\n",
			// cviFrameBuf.width, cviFrameBuf.height);

			write_yuv_file(outFileName,
				       (unsigned char *)&cviFrameBuf, i);
		} else {
			CVIBUF cviBuf;
			memset(&cviBuf, 0, sizeof(CVIBUF));
			ret = CVIJpgGetFrameData(jpgHandle,
						 (unsigned char *)&cviBuf,
						 sizeof(CVIBUF), NULL);
			write_jpg_file(outFileName, (unsigned char *)&cviBuf,
				       i);
		}
	}

FIND_ERROR:
	if (NULL != jpgHandle) {
		CVIJpgClose(jpgHandle);
		jpgHandle = NULL;
	}

	CVIJpgUninit();
#ifdef ENC_TYPE2
	if (0 != frame_buffer.vbY.virt_addr)
		vfree((unsigned char *)(frame_buffer.vbY.virt_addr));
	if (0 != frame_buffer.vbCb.virt_addr)
		vfree((unsigned char *)(frame_buffer.vbCb.virt_addr));
	if (0 != frame_buffer.vbCr.virt_addr)
		vfree((unsigned char *)(frame_buffer.vbCr.virt_addr));
#else
	if (NULL != srcBuf) {
		vfree(srcBuf);
		srcBuf = NULL;
	}
#endif
	vfree(inFileName);
	vfree(outFileName);
	return 0;
}

static void welcome(void)
{
	if (bQuiet) {
		return;
	}
	pr_info("// CVITEK JPEG test\n");
}

/* input and output param description */
static void usage(void)
{
	pr_info("usage:ask owner\n");
	pr_info("\n");
}

static int read_jpeg_file(char *inFileName, unsigned char **pSrcBuf)
{
	int srcLen = 0;
	int readLen = 0;
	if (0 != strcmp(inFileName, "")) {
		osal_file_t fpSrc = osal_fopen(inFileName, "rb");
		if (NULL == fpSrc) {
			pr_err("Cann't open input file %s\n", inFileName);
			goto FIND_ERROR;
		}
		/* get file size */
		osal_fseek(fpSrc, 0, SEEK_END);
		srcLen = osal_ftell(fpSrc);
		osal_fseek(fpSrc, 0, SEEK_SET);
		*pSrcBuf = vmalloc(srcLen + 16);
		readLen = osal_fread(*pSrcBuf, sizeof(unsigned char), srcLen, fpSrc);
		if (srcLen != readLen) {
			pr_info("Some Wrong happend at read jpeg file??, readLen = %d, request = %d\n",
			       readLen, srcLen);
		}
		pr_info("readLen = 0x%X\n", readLen);
		osal_fclose(fpSrc);
	}

FIND_ERROR:
	return readLen;
}

static int write_yuv_file(char *name, unsigned char *pDstBuf, int suffix)
{
	int i;
	int writeLen = 0;
	int writeTotalLen = 0;
	char writeName[256];
	osal_file_t fpYuv;
	int iChromaHeight;
	int iChromaWidth;

	CVIFRAMEBUF *cviFrameBuf = (CVIFRAMEBUF *)pDstBuf;
	unsigned char *addrY_v = (unsigned char *)(cviFrameBuf->vbY.virt_addr);
	unsigned char *addrCb_v =
		(unsigned char *)(cviFrameBuf->vbCb.virt_addr);
	unsigned char *addrCr_v =
		(unsigned char *)(cviFrameBuf->vbCr.virt_addr);

	unsigned char *address = addrY_v;
	int datLen = cviFrameBuf->width;

	sprintf(writeName, "%s%d.yuv", name, suffix);
	fpYuv = osal_fopen(writeName, "wb");
	if (fpYuv == 0) {
		pr_err("Cann't create a file to write data\n");
		return 0;
	}

	if (0 != iPackedFormat) {
		if ((5 == iPackedFormat) || (6 == iPackedFormat))
			datLen *= 3;
		else
			datLen *= 2;
	}

#if OUTPUT_FOR_SCALER
	pr_err("strideY = %d\n", cviFrameBuf->strideY);
	writeLen = osal_fwrite((unsigned char *)address, sizeof(unsigned char),
			  cviFrameBuf->height * cviFrameBuf->strideY, fpYuv);
#else
	for (i = 0; i < cviFrameBuf->height; i++) {
		writeLen = osal_fwrite((unsigned char *)address,
				  sizeof(unsigned char), datLen, fpYuv);
		writeTotalLen += writeLen;
		address = address + cviFrameBuf->strideY;
	}
#endif

	if (0 != iPackedFormat) {
		pr_err("iPackedFormat = %d\n", iPackedFormat);
		goto ERROR_WRITE_YUV;
	}

	iChromaHeight = cviFrameBuf->height;
	iChromaWidth = cviFrameBuf->width;
	switch (cviFrameBuf->format) {
	case CVI_FORMAT_422:
		iChromaHeight = cviFrameBuf->height;
		iChromaWidth = cviFrameBuf->width >> 1;
		break;
	case CVI_FORMAT_224:
		iChromaHeight = cviFrameBuf->height >> 1;
		iChromaWidth = cviFrameBuf->width;
		break;
	case CVI_FORMAT_444:
		iChromaHeight = cviFrameBuf->height;
		iChromaWidth = cviFrameBuf->width;
		break;
	case CVI_FORMAT_400:
		iChromaHeight = 0;
		iChromaWidth = 0;
		break;
	case CVI_FORMAT_420:
	default:
		iChromaHeight = cviFrameBuf->height >> 1;
		iChromaWidth = cviFrameBuf->width >> 1;
		break;
	}

#if OUTPUT_FOR_SCALER
	address = addrCb_v;

	pr_info("iChromaInterLeave = %d, iChromaHeight = %d, strideC = %d\n",
	       iChromaInterLeave, iChromaHeight, cviFrameBuf->strideC);

	writeLen = osal_fwrite((unsigned char *)address, sizeof(unsigned char),
			  iChromaHeight * cviFrameBuf->strideC, fpYuv);

	address = addrCr_v;

	writeLen = osal_fwrite((unsigned char *)address, sizeof(unsigned char),
			  iChromaHeight * cviFrameBuf->strideC, fpYuv);
#else
	address = addrCb_v;
	if (0 == iChromaInterLeave) {
		for (i = 0; i < iChromaHeight; i++) {
			writeLen = osal_fwrite((unsigned char *)address,
					  sizeof(unsigned char), iChromaWidth,
					  fpYuv);
			writeTotalLen += writeLen;
			address = address + cviFrameBuf->strideC;
		}

		address = addrCr_v;
		for (i = 0; i < iChromaHeight; i++) {
			writeLen = osal_fwrite((unsigned char *)address,
					  sizeof(unsigned char), iChromaWidth,
					  fpYuv);
			writeTotalLen += writeLen;
			address = address + cviFrameBuf->strideC;
		}
	} else {
		for (i = 0; i < iChromaHeight; i++) {
			writeLen = osal_fwrite((unsigned char *)address,
					  sizeof(unsigned char),
					  cviFrameBuf->width, fpYuv);
			writeTotalLen += writeLen;
			address = address + cviFrameBuf->strideC;
		}
	}
#endif

ERROR_WRITE_YUV:
	if (0 != fpYuv) {
		osal_fclose(fpYuv);
		fpYuv = NULL;
	}
	return writeTotalLen;
}

#if ENC_TYPE == 1

static int read_yuv_file(char *inFileName, unsigned char **pSrcBuf,
			 CVIFrameFormat fmt)
{
	int srcLen = 0;
	int readLen = 0;

	if (0 != strcmp(inFileName, "")) {
		FILE *fpSrc = osal_fopen(inFileName, "rb");
		if (NULL == fpSrc) {
			pr_err("Cann't open input file %s\n", inFileName);
			goto FIND_ERROR;
		}
		/* get file size */
		osal_fseek(fpSrc, 0, SEEK_END);
		srcLen = osal_ftell(fpSrc);
		osal_fseek(fpSrc, 0, SEEK_SET);
		*pSrcBuf = vmalloc(srcLen + 16);
		readLen = osal_fread(*pSrcBuf, sizeof(unsigned char), srcLen, fpSrc);
		if (srcLen != readLen) {
			pr_err("Some Wrong happend at read yuv file??, readLen = %d, request = %d\n",
			       readLen, srcLen);
		}
		osal_fclose(fpSrc);
	}
FIND_ERROR:
	return readLen;
}

#elif ENC_TYPE == 2

static int read_yuv_file2(char *inFileName, unsigned char *pSrcBuf,
			  CVIFrameFormat fmt)
{
	osal_file_t fpSrc = NULL;
	int readLen = 0;
	int dataLen = 0;
	CVIFRAMEBUF *buffer = (CVIFRAMEBUF *)pSrcBuf;
	if (NULL == buffer) {
		pr_err("alloc enc memory fail\n");
		goto FIND_ERROR;
	}

	if (0 != strcmp(inFileName, "")) {
		fpSrc = osal_fopen(inFileName, "rb");
		if (NULL == fpSrc) {
			pr_err("Cann't open input file %s\n", inFileName);
			goto FIND_ERROR;
		}
		int datLen = buffer->width;
		if (0 != iPackedFormat) {
			if ((5 == iPackedFormat) || (6 == iPackedFormat))
				datLen *= 3;
			else
				datLen *= 2;
		}
		readLen = osal_fread((unsigned char *)(buffer->vbY.virt_addr),
				sizeof(unsigned char), datLen * buffer->height,
				fpSrc);
		if (0 != iPackedFormat)
			goto FIND_ERROR;

		// [0](4:2:0) [1](4:2:2) [2](2:2:4 4:2:2 rotated) [3](4:4:4)
		// [4](4:0:0)
		dataLen = buffer->width * buffer->height;
		if (CVI_FORMAT_422 == buffer->Format ||
		    CVI_FORMAT_224 == buffer->Format) {
			dataLen = dataLen >> 1;
		} else if (CVI_FORMAT_400 == buffer->Format) {
			dataLen = 0;
		} else if (CVI_FORMAT_420 == buffer->Format) {
			dataLen = dataLen >> 2;
		}

		readLen += osal_fread((unsigned char *)(buffer->vbCb.virt_addr),
				 sizeof(unsigned char), dataLen, fpSrc);
		readLen += osal_fread((unsigned char *)(buffer->vbCr.virt_addr),
				 sizeof(unsigned char), dataLen, fpSrc);
	}
FIND_ERROR:
	if (NULL != fpSrc)
		osal_fclose(fpSrc);
	return readLen;
}

#elif ENC_TYPE == 3

static int read_yuv_file3(CVIJpgHandle jpgHandle, char *inFileName,
			  CVIEncConfigParam encConfig)
{
	int ret = 0;
	CVIFRAMEBUF DataBuf;
	unsigned char *pWriteData = 0;
	int iReadData = 0;
	unsigned char *pReadData;
	int i = 0;
	osal_file_t fpSrc;
	int nY = encConfig.picHeight;
	int nCb = 0;
	int nCr = 0;
	int chromaWidth = 0;
	int picWidth = encConfig.picWidth;

	memset(&DataBuf, 0, sizeof(CVIFRAMEBUF));
	ret = CVIJpgGetInputDataBuf(jpgHandle, &DataBuf, sizeof(CVIFRAMEBUF));
	if (0 != ret) {
		pr_err("\nFailed to CVIJpgGetInputDataBuf, ret = %d\n", ret);
		return 0;
	}

	switch (encConfig.sourceFormat) {
	case 0:
		nCb = nCr = encConfig.picHeight >> 1;
		chromaWidth = encConfig.picWidth >> 1;
		break;
	case 1:
		nCb = nCr = encConfig.picHeight;
		chromaWidth = encConfig.picWidth >> 1;
		break;
	case 2:
		nCb = nCr = encConfig.picHeight >> 1;
		chromaWidth = encConfig.picWidth;
		break;
	case 3:
		nCb = nCr = encConfig.picHeight;
		chromaWidth = encConfig.picWidth;
		break;
	case 4:
		nCb = nCr = 0;
		chromaWidth = encConfig.picWidth >> 1;
		break;
	}

	if (encConfig.packedFormat) {
		if ((5 == encConfig.packedFormat) ||
		    (6 == encConfig.packedFormat))
			picWidth *= 3;
		else
			picWidth *= 2;
	}

	fpSrc = osal_fopen(inFileName, "rb");
	if (NULL == fpSrc) {
		pr_err("Cann't open input file %s\n", inFileName);
		return 0;
	}

	pReadData =
		vmalloc(encConfig.picWidth * sizeof(unsigned char) * 3 + 16);


	/* read Y */
	for (i = 0; i < nY; i++) {
		pWriteData = (unsigned char *)(DataBuf.vbY.virt_addr) +
			     DataBuf.strideY * i;
		iReadData += osal_fread(pReadData, sizeof(unsigned char), picWidth,
				   fpSrc);
		memcpy(pWriteData, pReadData, picWidth);
	}

	if ((4 == encConfig.sourceFormat) || encConfig.packedFormat) {
		vfree(pReadData);
		return iReadData;
	}

	if (encConfig.chromaInterleave) {
		/* read Cb */
		for (i = 0; i < nCb; i++) {
			pWriteData = (unsigned char *)(DataBuf.vbCb.virt_addr) +
				     DataBuf.strideC * i;
			iReadData += osal_fread(pReadData, sizeof(unsigned char),
					   chromaWidth * 2, fpSrc);
			memcpy(pWriteData, pReadData, chromaWidth * 2);
		}
	} else {
		/* read Cb */
		for (i = 0; i < nCb; i++) {
			pWriteData = (unsigned char *)(DataBuf.vbCb.virt_addr) +
				     DataBuf.strideC * i;
			iReadData += osal_fread(pReadData, sizeof(unsigned char),
					   chromaWidth, fpSrc);
			memcpy(pWriteData, pReadData, chromaWidth);
		}

		/* read Cr */
		for (i = 0; i < nCb; i++) {
			pWriteData = (unsigned char *)(DataBuf.vbCr.virt_addr) +
				     DataBuf.strideC * i;
			iReadData += osal_fread(pReadData, sizeof(unsigned char),
					   chromaWidth, fpSrc);
			memcpy(pWriteData, pReadData, chromaWidth);
		}
	}

	vfree(pReadData);
	osal_fclose(fpSrc);

	return iReadData;
}

#endif

static int write_jpg_file(char *name, unsigned char *pDstBuf, int suffix)
{
	// int i;
	int writeLen = 0;
	char writeName[256];
	CVIBUF *cviBuf = (CVIBUF *)pDstBuf;
	unsigned char *address = (unsigned char *)(cviBuf->virt_addr);
	unsigned int length = (int)(cviBuf->size);
	osal_file_t fpJpg;

	sprintf(writeName, "%s%d.jpg", name, suffix);
	fpJpg = osal_fopen(writeName, "wb");
	if (0 == fpJpg) {
		pr_err("Cann't create a file %s to write data\n", writeName);
		return 0;
	}

	writeLen = osal_fwrite((unsigned char *)address, sizeof(unsigned char),
			  length, fpJpg);
	osal_fclose(fpJpg);
	fpJpg = NULL;

	return writeLen;
}


int cvi_jpeg_test(u_long arg)
{
#define MAX_ARG_CNT 30
	char buf[512];
	char *pArgv[MAX_ARG_CNT] = {0};
	char *save_ptr;
	unsigned int u32Argc = 0;
	char *pBuf;
	unsigned int __user *argp = (unsigned int __user *)arg;

	memset(buf, 0, 512);

	if (argp != NULL) {
		if (copy_from_user(buf, (char *)argp, 512))
			return -1;
	}

	pBuf = buf;

	while (NULL != (pArgv[u32Argc] = cvi_strtok_r(pBuf, " ", &save_ptr))) {
		u32Argc++;

		if (u32Argc >= MAX_ARG_CNT) {
			break;
		}

		pBuf = NULL;
	}

	return jpeg_main(u32Argc, pArgv);
}
#endif