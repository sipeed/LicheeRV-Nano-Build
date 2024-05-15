#ifndef __RTSP_SERVER_H
#define __RTSP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib.h"
#include "stdint.h"

int rtsp_server_init(char *ip, int port);
int rtsp_server_deinit(void);
char *rtsp_get_server_ip(void);
int rtsp_get_server_port(void);
int rtsp_server_start(void);
int rtsp_server_stop(void);
void rtsp_send_h265_data(uint8_t *asddata, size_t data_len);

#ifdef __cplusplus
}
#endif

#endif // __RTSP_SERVER_H
