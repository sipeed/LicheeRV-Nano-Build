#include "cvi_ive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int main(int argc, char **argv)
{
	if (argc != 5) {
		printf("Incorrect loop value. Usage: %s <w> <h> <file_name> <count>\n",
		       argv[0]);
		printf("Example: %s 352 288 data/00_352x288_444.yuv 100\n", argv[0]);
		return CVI_FAILURE;
	}
	// 00_352x288_444.yuv
	const char *filename = argv[3];
	int ret = CVI_SUCCESS;
	int input_w, input_h;
	struct timeval t0, t1;
	unsigned long elapsed_cpu, count;

	input_w = atoi(argv[1]);
	input_h = atoi(argv[2]);
	IVE_HANDLE handle = CVI_IVE_CreateHandle();

	// Create src image.
	IVE_IMAGE_S src_444;

	CVI_IVE_ReadRawImage(handle, &src_444, filename,
			  IVE_IMAGE_TYPE_U8C3_PLANAR, input_w, input_h);

	// Create dst image.
	IVE_DST_IMAGE_S dst;

	CVI_IVE_CreateImage(handle, &dst, IVE_IMAGE_TYPE_U8C3_PACKAGE, input_w,
			    input_h);

	// Config Setting.
	IVE_CSC_CTRL_S stCtrl;

	stCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
	count = atoi(argv[4]);

	// Run HW IVE.
	printf("Run HW IVE CSC YUV2RGB.\n");
	gettimeofday(&t0, NULL);
	while (count > 0) {
		ret |= CVI_IVE_CSC(handle, &src_444, &dst, &stCtrl, 1);
		count--;
	}
	gettimeofday(&t1, NULL);
	elapsed_cpu =
		((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
	printf("%s CPU time %lu\n", __func__, elapsed_cpu);

	CVI_IVE_WriteImg(handle, "sample_CSC_YUV2RGB.rgb", &dst);

	CVI_SYS_FreeI(handle, &src_444);
	CVI_SYS_FreeI(handle, &dst);
	CVI_IVE_DestroyHandle(handle);

	return ret;
}
