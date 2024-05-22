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

#include "sample_comm.h"
#include "sample_vio.h"
#include "cvi_sys.h"
#include <linux/cvi_type.h>

void SAMPLE_VIO_HandleSig(CVI_S32 signo)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	if (SIGINT == signo || SIGTERM == signo) {
		//todo for release
		SAMPLE_PRT("Program termination abnormally\n");
	}
	exit(-1);
}

void SAMPLE_VIO_Usage(char *sPrgNm)
{
	printf("Usage : %s <index>\n", sPrgNm);
	printf("index:\n");
	printf("\t 0) test vo only.\n");
	printf("\t 1) test vi only.\n");
	printf("\t 2) test vi vo.\n");
	printf("\t 3) test region.\n");
	printf("\t 4) test venc jpg.\n");
	printf("\t 5) test venc h265.\n");
	printf("\t 6) test vi venc h265.\n");
	printf("\t 7) test rtsp h264.\n");
	printf("\t 8) test venc h264.\n");
	printf("\t 9) test rtsp h265.\n");
	printf("\t 10) test vi venc h265 rtsp.\n");
	printf("\t 11) test multiple vi.\n");
	printf("\t 12) test vi venc region h265 rtsp.\n");
	printf("\t 13) test i2c oled.\n");
}

int main(int argc, char *argv[])
{
	CVI_S32 s32Ret = CVI_FAILURE;
	CVI_S32 s32Index;

	if (argc < 2) {
		SAMPLE_VIO_Usage(argv[0]);
		return CVI_FAILURE;
	}

	if (!strncmp(argv[1], "-h", 2)) {
		SAMPLE_VIO_Usage(argv[0]);
		return CVI_SUCCESS;
	}

	extern int test_pre_init(void);
	extern int test_vo_only(void);
	extern int test_vi_only(void);
	extern int test_vio(void);
	extern int test_region(void);
	extern int test_venc_jpg(void);
	extern int test_venc_h265(void);
	extern int test_vi_venc_h265(void);
	extern int test_rtsp_h264(void);
	extern int test_vi_venc_h264(void);
	extern int test_rtsp_h265(void);
	extern int test_vi_venc_h265_rtsp(void);
	extern int test_multiple_vi(void);
	extern int test_vi_region_venc_h265_rtsp(void);
	extern int test_i2c_oled(void);

	test_pre_init();

	s32Index = atoi(argv[1]);
	switch (s32Index) {
	case 0:
		s32Ret = test_vo_only();
		break;
	case 1:
		s32Ret = test_vi_only();
		break;
	case 2:
		s32Ret = test_vio();
		break;
	case 3:
		s32Ret = test_region();
		break;
	case 4:
		s32Ret = test_venc_jpg();
		break;
	case 5:
		s32Ret = test_venc_h265();
		break;
	case 6:
		s32Ret = test_vi_venc_h265();
		break;
	case 7:
		s32Ret = test_rtsp_h264();
		break;
	case 8:
		s32Ret = test_vi_venc_h264();
		break;
	case 9:
		s32Ret = test_rtsp_h265();
		break;
	case 10:
		s32Ret = test_vi_venc_h265_rtsp();
		break;
	case 11:
		s32Ret = test_multiple_vi();
		break;
	case 12:
		s32Ret = test_vi_region_venc_h265_rtsp();
		break;
	case 13:
		s32Ret = test_i2c_oled();
		break;
	default:
		SAMPLE_PRT("the index %d is invaild!\n", s32Index);
		SAMPLE_VIO_Usage(argv[0]);
		return CVI_FAILURE;
	}

	if (s32Ret == CVI_SUCCESS)
		SAMPLE_PRT("sample_vio exit success!\n");
	else
		SAMPLE_PRT("sample_vio exit abnormally!\n");

	return s32Ret;
}

git filter-repo -f  --index-filter 'git rm -rf --cached --ignore-unmatch ./middleware/v2/sample/test_mmf/sophgo_middleware.c' HEAD

git filter-repo