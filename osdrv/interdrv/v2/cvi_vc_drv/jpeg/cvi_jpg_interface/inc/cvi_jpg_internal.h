#ifndef CVI_JPG_INTERNAL_H
#define CVI_JPG_INTERNAL_H

/* default jpu device struct */
#define WIDTH_BUFFER_SIZE 4096
#define HEIGHT_BUFFER_SIZE 2220

#define MEM_MALLOC(SIZE) vmalloc(SIZE)
#define MEM_CALLOC(NUMBER, SIZE) vzalloc(SIZE * NUMBER)
#define MEM_KMALLOC(SIZE) kmalloc(SIZE, GFP_KERNEL)
#define MEM_FREE(PTR) vfree(PTR)
#define MEM_KFREE(PTR) kfree(PTR)

/* DEC */
int cviJpgDecOpen(CVIJpgHandle *pHandle, CVIDecConfigParam *pConfig);
int cviJpgDecClose(CVIJpgHandle handle);
int cviJpgDecSendFrameData(CVIJpgHandle jpgHandle, void *data, int length);
int cviJpgDecGetFrameData(CVIJpgHandle jpgHandle, void *data);
int cviJpgDecFlush(CVIJpgHandle jpgHandle);

/* ENC */
int cviJpgEncOpen(CVIJpgHandle *pHandle, CVIEncConfigParam *pConfig);
int cviJpgEncClose(CVIJpgHandle handle);
int cviJpgEncSendFrameData(CVIJpgHandle jpgHandle, void *data, int srcType);
int cviJpgEncGetFrameData(CVIJpgHandle jpgHandle, void *data);
int cviJpgEncFlush(CVIJpgHandle jpgHandle);
int cviJpgEncGetInputDataBuf(CVIJpgHandle jpgHandle, void *data);
int cviJpgEncResetQualityTable(CVIJpgHandle jpgHandle);
int cviJpgEncEncodeUserData(CVIJpgHandle jpgHandle, void *data);
int cviJpgEncStart(CVIJpgHandle jpgHandle, void *data);

extern void jpu_set_channel_num(int chnIdx);

#endif /* CVI_JPG_INTERNAL_H */
