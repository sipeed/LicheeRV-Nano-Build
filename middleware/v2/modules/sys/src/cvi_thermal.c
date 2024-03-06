#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/socket.h>
#include <linux/genetlink.h>
#include <linux/thermal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "cvi_sys.h"

/* Generic macros for dealing with netlink sockets. Might be duplicated
 * elsewhere. It is recommended that commercial grade applications use
 * libnl or libnetlink and use the interfaces provided by the library
 */
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char *)(na) + NLA_HDRLEN))
#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

//Variables used for netlink
struct sockaddr_nl nl_address; //netlink socket address

struct nlattr *nl_na; //pointer to netlink attributes structure within the payload
struct { //memory for netlink request and response messages - headers are included
	struct nlmsghdr n;
	struct genlmsghdr g;
	char buf[256];
} nl_request_msg, nl_response_msg;

struct thermal_event {
	int orig;
	unsigned short up_down;
	unsigned short trip_id;
} thermal_event;
struct thermal_event *tz_evt;

typedef void (*event_cb_t)(int fps);

pthread_t thermal_thread;
CVI_BOOL g_is_thermal_running = CVI_FALSE;
event_cb_t callbackFunc;
int therm_sockets[2];

static int _init_bind_genl_socket(void)
{
	//Step 1: Open the socket. Note that protocol = NETLINK_GENERIC
	int nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);

	if (nl_fd < 0) {
		perror("socket()");
		return -1;
	}

	//Step 2: Bind the socket.
	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;
	nl_address.nl_groups = 0;

	if (bind(nl_fd, (struct sockaddr *) &nl_address, sizeof(nl_address)) < 0) {
		perror("bind()");
		close(nl_fd);
		return -1;
	}
	return nl_fd;
}

static int _get_mcast_grpid_genl_socket(int nl_fd, const char *family_name)
{
	int nl_group_id = 0;
	int nl_rxtx_length;

	//Step 3. Resolve the family ID corresponding to the string "thermal_event"
	//Populate the netlink header
	nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
	nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
	nl_request_msg.n.nlmsg_seq = 0;
	nl_request_msg.n.nlmsg_pid = getpid();
	nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	//Populate the payload's "family header" : which in our case is genlmsghdr
	nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;
	nl_request_msg.g.version = 0x1;
	//Populate the payload's "netlink attributes"
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);
	nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;
	nl_na->nla_len = strlen(family_name) + 1 + NLA_HDRLEN;
	strcpy(NLA_DATA(nl_na), family_name); //Family name length can be upto 16 chars including \0

	nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	//Send the family ID request message to the netlink controller
	nl_rxtx_length = sendto(nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,
	0, (struct sockaddr *) &nl_address, sizeof(nl_address));
	if (nl_rxtx_length != (int) nl_request_msg.n.nlmsg_len) {
		perror("sendto()");
		close(nl_fd);
		return -1;
	}
	//Wait for the response message
	nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
	if (nl_rxtx_length < 0) {
		perror("recv()");
		return -1;
	}

	//Validate response message
	if (!NLMSG_OK((&nl_response_msg.n), (unsigned int) nl_rxtx_length)) {
		fprintf(stderr, "family ID request : invalid message\n");
		return -1;
	}
	if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //error
		fprintf(stderr, "family ID request : receive error\n");
		return -1;
	}

	//Extract family ID
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
	nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));

#if 0
	if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {
		int nl_family_id = *(__u16 *) NLA_DATA(nl_na);

		fprintf(stderr, "family ID get : %u\n", nl_family_id);
	}
#endif

	do {
	//CTRL_ATTR_VERSION
	//CTRL_ATTR_HDRSIZE
	//CTRL_ATTR_MAXATTR
	//CTRL_ATTR_MCAST_GROUPS
		nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
	} while (*((char *) nl_na) != 0 && nl_na->nla_type != CTRL_ATTR_MCAST_GROUPS);

	// fprintf(stderr, "family Mcast group get : %d len %u\n", nl_na->nla_type, nl_na->nla_len);
	// for (unsigned short i = 0;i < nl_na->nla_len;i++)
	// {
	//     printf("%02x ", *(((char *) nl_na)+i));
	//     if (i % 16 == 15) printf("\n");
	// }
	// printf("\n");
	if (nl_na->nla_type == CTRL_ATTR_MCAST_GROUPS) {
		nl_na = (struct nlattr *) NLA_DATA(nl_na);
		// fprintf(stderr, "family Mcast group get NLA : %d len %u\n", nl_na->nla_type, nl_na->nla_len);
		nl_na = (struct nlattr *) NLA_DATA(nl_na);
		if (nl_na->nla_type == CTRL_ATTR_MCAST_GRP_ID) {
			nl_group_id = *(__u32 *) NLA_DATA(nl_na);
			// fprintf(stderr, "family Mcast grp id get : %d\n", nl_group_id);
			nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
		}
		if (nl_na->nla_type == CTRL_ATTR_MCAST_GRP_NAME) {
			// fprintf(stderr, "family Mcast grp name get : %s\n", (char *) NLA_DATA(nl_na));
			nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
		}
#ifdef THERMAL_GENL_EVENT_GROUP_NAME
		nl_na = (struct nlattr *) NLA_DATA(nl_na);
		if (nl_na->nla_type == CTRL_ATTR_MCAST_GRP_ID) {
			nl_group_id = *(__u32 *) NLA_DATA(nl_na);
			// fprintf(stderr, "family Mcast grp id get : %d\n", nl_group_id);
			nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
		}
		if (nl_na->nla_type == CTRL_ATTR_MCAST_GRP_NAME) {
			// fprintf(stderr, "family Mcast grp name get : %s\n", (char *) NLA_DATA(nl_na));
		}
#endif
	}
	return nl_group_id;
}

static int _get_notify_genl_socket(int nl_fd, int *trip_id, int *up_down)
{
	int nl_rxtx_length; //Number of bytes sent or received via send() or recv()

	//Step 4. Send own custom message
	memset(&nl_response_msg, 0, sizeof(nl_response_msg));

	//Receive reply from kernel
	nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
	if (nl_rxtx_length < 0) {
		perror("recv()");
		return -1;
	}

	//Validate response message
	if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //Error
		CVI_TRACE_SYS(CVI_DBG_ERR, "Error while receiving reply from kernel: NACK Received\n");
		return -1;
	}
	if (nl_rxtx_length < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Error while receiving reply from kernel\n");
		return -1;
	}
	if (!NLMSG_OK((&nl_response_msg.n), (unsigned int) nl_rxtx_length)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Error while receiving reply from kernel: Invalid Message\n");
		return -1;
	}

	//Parse the reply message
	nl_rxtx_length = GENLMSG_PAYLOAD(&nl_response_msg.n);
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
	// fprintf(stderr, "Recv Response: type %d len %u\n", nl_na->nla_type, nl_na->nla_len);

#ifdef THERMAL_GENL_EVENT_GROUP_NAME
	if ((nl_response_msg.g.cmd != THERMAL_GENL_EVENT_TZ_TRIP_UP) &&
		(nl_response_msg.g.cmd != THERMAL_GENL_EVENT_TZ_TRIP_DOWN))
		return -1;

	if (nl_na->nla_type != THERMAL_GENL_ATTR_TZ_TRIP_ID) {
		// fprintf(stderr, "Response: type %d len %u\n", nl_na->nla_type, nl_na->nla_len);
		// for (unsigned short i = 0;i < nl_na->nla_len;i++)
		// {
		//     printf("%02x ", *(((char *) nl_na)+i));
		//     if (i % 16 == 15) printf("\n");
		// }
		// printf("\n");
		nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
	}

	*trip_id = *(__u32 *) NLA_DATA(nl_na);
	*up_down = (nl_response_msg.g.cmd == THERMAL_GENL_EVENT_TZ_TRIP_UP) ? 0 : 1;
#else
	tz_evt = (struct thermal_event *) NLA_DATA(nl_na);
	*trip_id = tz_evt->trip_id;
	*up_down = tz_evt->up_down;
#endif
	return 0;
}

static int _read_cooling_maxstate(void)
{
	CVI_CHAR buf[32];
	char temp_path[128] = "/sys/class/thermal/cooling_device0/max_state";
	int fd = open(temp_path, O_RDONLY);

	if (fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "open cooling max_state failed\n");
		return -1;
	}

	read(fd, buf, sizeof(buf));
	close(fd);
	return atoi(buf);
}

static int _write_cooling_curstate(int cur_state)
{
	CVI_CHAR buf[32];
	char temp_path[128] = "/sys/class/thermal/cooling_device0/cur_state";
	int fd = open(temp_path, O_WRONLY);

	if (fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "open cooling cur_state failed\n");
		return -1;
	}

	snprintf(buf, sizeof(buf), "%d", cur_state);
	write(fd, buf, sizeof(buf));
	close(fd);
	return 0;
}

static int _write_trip_pointers(int trip_id, int trip_temps, const char *target)
{
	CVI_CHAR buf[32];
	CVI_S32 trip_point_fd = 0;
	char temp_path[128] = "/sys/class/thermal/thermal_zone0/trip_point_?_";

	strcat(temp_path, target);
	temp_path[44] = 0x30 + trip_id;
	trip_point_fd = open(temp_path, O_WRONLY);
	if (trip_point_fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "open trip point temp failed\n");
		return -1;
	}
	snprintf(buf, sizeof(buf), "%d", trip_temps);
	write(trip_point_fd, buf, sizeof(buf));
	close(trip_point_fd);
	return 0;
}

static unsigned int _read_trip_pointers(int trip_temps[], unsigned int uplimit)
{
	CVI_CHAR buf[32];
	CVI_S32 trip_point_fd = 0;
	char temp_path[128] = "/sys/class/thermal/thermal_zone0/trip_point_?_temp";
	char type_path[128] = "/sys/class/thermal/thermal_zone0/trip_point_?_type";
	unsigned int trip_id;

	for (trip_id = 0; trip_id < uplimit; trip_id++) {
		temp_path[44] = 0x30 + trip_id;
		type_path[44] = 0x30 + trip_id;
		trip_point_fd = open(temp_path, O_RDONLY);
		if (trip_point_fd < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "open trip point temp failed\n");
			break;
		}

		read(trip_point_fd, buf, sizeof(buf));
		close(trip_point_fd);
		trip_temps[trip_id] = strtol(buf, NULL, 10);
		trip_point_fd = open(type_path, O_RDONLY);
		if (trip_point_fd < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "open trip point type failed\n");
			break;
		}
		read(trip_point_fd, buf, sizeof(buf));
		close(trip_point_fd);
		if (strncmp(buf, "critical", 8) == 0) {
			trip_temps[trip_id] = 0;
			break;
		}
	}

	return trip_id;
}

static int _read_chip_temp(CVI_S32 *temp)
{
	CVI_CHAR buf[32];
	CVI_S32 thermal_fd = 0;

	thermal_fd = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
	if (thermal_fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "open thermal failed\n");
		return -1;
	}

	read(thermal_fd, buf, sizeof(buf));
	close(thermal_fd);

	*temp = strtol(buf, NULL, 10);
	return 0;
}

static void _log_event(FILE *fpEvtlog, int trip_id, int up_down, int fps, int state)
{
	time_t rawtime;
	struct tm *info;
	char buffer[80];
	char str_buffer[128];
	int ret;
	CVI_S32 temp = 0;

	if (!fpEvtlog)
		return;

	_read_chip_temp(&temp);
	time(&rawtime);
	info = localtime(&rawtime);
	strftime(buffer, 80, "%y/%m/%d %X", info);
	if (trip_id < 0) {
		ret = snprintf(str_buffer, 128, "[%s] temp %d\n", buffer, temp);
	} else if (fps <= 0) {
		ret = snprintf(str_buffer, 128, "[%s] temp %d trip_id %d %s. %sCoolingState %d\n"
						, buffer, temp, trip_id, (up_down) ? "down" : "up"
						, (fps == 0) ? "Restore FPS, " : "", state);
	} else {
		ret = snprintf(str_buffer, 128, "[%s] temp %d trip_id %d %s. Set FPS:%d CoolingState %d\n"
						, buffer, temp, trip_id, (up_down) ? "down" : "up"
						, fps, state);
	}
	fwrite(str_buffer, ret, 1, fpEvtlog);
	fflush(fpEvtlog);
}

static void _dump_register(FILE *fpEvtlog)
{
	FILE *fpReadReg = NULL;

	if (!fpEvtlog)
		return;

	fpReadReg = fopen("/sys/class/cvi-base/base_efuse_shadow", "r");
	if (fpReadReg) {
		char str_buffer[128] = "dump register:\n";
		int ahead = 0, read;

		fwrite(str_buffer, strlen(str_buffer), 1, fpEvtlog);
		do {
			char out_buffer[128];
			int out = 0;

			read = fread(str_buffer, 1, sizeof(str_buffer), fpReadReg);
			// printf("fread %d byte\n", read);
			for (int j = 0; j < read; j++) {
				if ((j%16) == 0)
					out = snprintf(out_buffer, 128, "%04X  ", j + ahead);
				out += snprintf(out_buffer + out, 128 - out, "%02X ", str_buffer[j]);
				if ((j%16) == 15) {
					out += snprintf(out_buffer + out, 128 - out, "\n");
					fwrite(out_buffer, out, 1, fpEvtlog);
					out = 0;
				}
			}
			ahead = read;
		} while (read == sizeof(str_buffer));
		fclose(fpReadReg);
		fflush(fpEvtlog);
	}
}

void CVI_SYS_RegisterThermalCallback(void (*setFPS)(int))
{
	callbackFunc = setFPS;
}

static CVI_VOID *tz_event_handler(CVI_VOID *data)
{
	int nl_fd;  //netlink socket's file descriptor
	int nl_group_id;
	int err;
	int trip_temps[10];
	CVI_U32 trip_num;
	CVI_S32 init_temp = 0;
	int FPS_ARRAY[4] = {0, 15, 5, 1};
	int COOLING_ARRAY[4] = {0, 1, 2, 3};
	int FPS;
	CVI_U32 zone;
	CVI_U32 i;
	int max_state;
	char *CtrlEnv = getenv("cvitherm_ctl");
	char *EvtlogEnv = getenv("cvitherm_log");
	char *EvtlogInterval = getenv("cvitherm_interval");
	FILE *fpEvtlog = NULL;
	int iEvtlogInterval = (EvtlogInterval) ? atoi(EvtlogInterval) : 10;

	prctl(PR_SET_NAME, "tz_event_handler");
	(void)(data);

	nl_fd = _init_bind_genl_socket();
	if (nl_fd < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "init netlink socket fail\n");
		goto exit_loop;
	}

	nl_group_id = _get_mcast_grpid_genl_socket(nl_fd, THERMAL_GENL_FAMILY_NAME);
	if (nl_group_id == 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "get mcast grp id fail\n");
		close(nl_fd);
		goto exit_loop;
	}

	err = setsockopt(nl_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &nl_group_id, sizeof(nl_group_id));
	if (err < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "NETLINK_ADD_MEMBERSHIP fail\n");
		close(nl_fd);
		goto exit_loop;
	}

	if (_read_chip_temp(&init_temp) < 0) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "Read init temp fail\n");
		close(nl_fd);
		goto exit_loop;
	}

	max_state = _read_cooling_maxstate();
	trip_num = _read_trip_pointers(trip_temps, sizeof(trip_temps)/sizeof(int));

	if (EvtlogEnv) {
		fpEvtlog = fopen(EvtlogEnv, "a");
		_dump_register(fpEvtlog);
	}
	if (CtrlEnv) {
		char *colon = strchr(CtrlEnv, ':');
		char *newString = NULL, *newString2 = NULL;
		char *comma = CtrlEnv;

		if (colon) {
			newString = strndup(CtrlEnv, colon - CtrlEnv);
			comma = colon;
			colon = strchr(colon+1, ':');
			if (colon) {
				newString2 = strndup(comma, colon - comma);
				comma = colon+1;
				for (i = 1; comma && (*comma) && i < sizeof(COOLING_ARRAY)/sizeof(int); i++) {
					COOLING_ARRAY[i] = atoi(comma);
					comma = strchr(comma, ',');
					if (comma)
						comma++;
				}
				comma = newString2;
			}
			comma++;
			for (i = 1; comma && (*comma) && i < sizeof(FPS_ARRAY)/sizeof(int); i++) {
				FPS_ARRAY[i] = atoi(comma);
				comma = strchr(comma, ',');
				if (comma)
					comma++;
			}
			comma = newString;
		}
		for (i = 0; comma && (*comma) && i < trip_num; i++) {
			int hyst;

			trip_temps[i] = atoi(comma)*1000;
			_write_trip_pointers(i, trip_temps[i], "temp");
			comma = strchr(comma, ',');
			if (comma)
				comma++;
			hyst = atoi(comma)*1000;
			_write_trip_pointers(i, hyst, "hyst");
			comma = strchr(comma, ',');
			if (comma)
				comma++;
		}
		if (newString)
			free(newString);
		if (newString2)
			free(newString2);
	}
#if 0
	printf("FPS: ");
	for (i = 0; i < sizeof(FPS_ARRAY)/sizeof(int); i++) {
		printf("[%d] ", FPS_ARRAY[i]);
	}
	printf("\n");
	printf("TRIPs: ");
	for (i = 0; i < trip_num; i++) {
		printf("[%d] ", trip_temps[i]);
	}
	printf("\n");
	printf("COOLINGs: ");
	for (i = 0; i < sizeof(COOLING_ARRAY)/sizeof(int); i++) {
		printf("[%d] ", COOLING_ARRAY[i]);
	}
	printf("\n");
#endif
	for (zone = 0; zone < trip_num; zone++) {
		if (init_temp < trip_temps[zone]) {
			break;
		}
	}
	//Not zero zone
	if (zone > 1) {
		FPS = FPS_ARRAY[zone];
		if (callbackFunc)
			callbackFunc(FPS);
	}
	_write_cooling_curstate((COOLING_ARRAY[zone] > max_state) ? max_state : COOLING_ARRAY[zone]);
	CVI_TRACE_SYS(CVI_DBG_INFO, "Read init_temp %d max_state %d trip_point_num %d zone %d\n"
		, init_temp, max_state, trip_num, zone);

	while (g_is_thermal_running) {
		int trip_id, up_down;
		int up_check, down_check;
		CVI_U32 next_zone = zone;
		int r;
		int max_fd = nl_fd;
		fd_set readfds;
		struct timeval tv;

		FD_ZERO(&readfds);
		FD_SET(nl_fd, &readfds);
		FD_SET(therm_sockets[1], &readfds);
		tv.tv_sec = iEvtlogInterval;
		tv.tv_usec = 0;
		if (therm_sockets[1] > max_fd)
			max_fd = therm_sockets[1];
		r = select(max_fd + 1, &readfds, NULL, NULL, &tv);

		if (r == -1) {
			if (errno == EINTR)
				continue;
			continue;
		} else if (r == 0) {
			//fprintf(stderr, "%s: select timeout\n", __func__);
			_log_event(fpEvtlog, -1, -1, -1, -1);
			continue;
		} else if (FD_ISSET(therm_sockets[1], &readfds)) {
			break;
		}

		if (_get_notify_genl_socket(nl_fd, &trip_id, &up_down) < 0) {
			CVI_TRACE_SYS(CVI_DBG_ERR, "_get_notify_genl_socket fail\n");
			continue;
		}
		CVI_TRACE_SYS(CVI_DBG_INFO, "Got trip %d %s\n", trip_id, (up_down) ? "down" : "up");
		down_check = zone - 1;
		up_check = zone;
		if (down_check == trip_id && up_down == 1) {
			next_zone = zone - 1;
		} else if (trip_id < down_check && up_down == 1) {
			next_zone = trip_id;
		} else if (up_check == trip_id && up_down == 0) {
			next_zone = zone + 1;
		} else if (trip_id > up_check && up_down == 0) {
			next_zone = trip_id + 1;
		}
		if (next_zone != zone && next_zone < sizeof(COOLING_ARRAY)/sizeof(int)) {
			int next_state = COOLING_ARRAY[next_zone];

			if (next_state > max_state)
				next_state = max_state;
			_write_cooling_curstate(next_state);
			FPS = FPS_ARRAY[next_zone];
			if (callbackFunc) {
				callbackFunc(FPS);
				_log_event(fpEvtlog, trip_id, up_down, FPS, next_state);
			} else {
				_log_event(fpEvtlog, trip_id, up_down, -1, next_state);
			}
			zone = next_zone;
		}
	}
	if (fpEvtlog) {
		fclose(fpEvtlog);
		fpEvtlog = NULL;
	}
	close(nl_fd);

exit_loop:
	g_is_thermal_running = CVI_FALSE;
	pthread_exit(NULL);
}

CVI_S32 CVI_SYS_StartThermalThread(void)
{
	struct sched_param param;
	pthread_attr_t attr;

	if (g_is_thermal_running == CVI_TRUE) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "already started\n");
		return CVI_FAILURE;
	}
	g_is_thermal_running = CVI_TRUE;
	socketpair(AF_UNIX, SOCK_SEQPACKET, 0, therm_sockets);

	param.sched_priority = 85;

	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	pthread_create(&thermal_thread, &attr, (void *)tz_event_handler, NULL);
	CVI_TRACE_SYS(CVI_DBG_INFO, "CVI_SYS_StartThermalThread\n");
	return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_StopThermalThread(void)
{
	if (g_is_thermal_running == CVI_FALSE) {
		CVI_TRACE_SYS(CVI_DBG_WARN, "not start yet\n");
		return CVI_FAILURE;
	}

	g_is_thermal_running = CVI_FALSE;
	if (thermal_thread) {
		char buf[8];

		write(therm_sockets[0], buf, 8);
		pthread_join(thermal_thread, NULL);
		thermal_thread = 0;
	}
	close(therm_sockets[0]);
	close(therm_sockets[1]);
	CVI_TRACE_SYS(CVI_DBG_INFO, "CVI_SYS_StopThermalThread\n");
	return CVI_SUCCESS;
}
