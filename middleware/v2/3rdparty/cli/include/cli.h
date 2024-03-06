#ifndef __CLIAPI_H__
#define __CLIAPI_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define CLI_UART_SUPPORT 0x01
#define CLI_TELNET_SUPPORT 0x02
#define CLI_UART_CLIENT_SUPPORT 0x04


#define DECLARE_TELNET_CLI_CMD_MACRO(name, child, Cbfunc, help, usermode) {     \
(#name),                                                      \
child,                                                      \
Cbfunc,                                                     \
(#help),                                                      \
usermode,                                                   \
0,                                                    \
0,                                                   \
0                                                    \
}

#define DECLARE_TELNET_CLI_CMD_MACRO_END() {              \
(0),                                                   \
(0),                                                   \
(0),                                                   \
(0),                                                   \
(0),                                                   \
(0),                                                    \
(0),                                                   \
(0)                                                   \
}

typedef enum{
	DEBUG_EN_TRACE_NONE = 0, //always print
	DEBUG_EN_TRACE_ERROR,
	DEBUG_EN_TRACE_WARNING,
	DEBUG_EN_TRACE_INFO,
	DEBUG_EN_TRACE_BUTT
} DEBUG_EN_LEVEL;

typedef struct TELNET_CLI_S_COMMANDtag {
	char *pcName;
	struct TELNET_CLI_S_COMMANDtag *pChildren;
	int (*pCallback)(int argc, char *argv[]);
	char *pcCmdHelp;
	int i32UserMode;
	struct TELNET_CLI_S_COMMANDtag *next;
	struct TELNET_CLI_S_COMMANDtag *parent;
	struct TELNET_CLI_S_COMMANDtag *prev;
} TELNET_CLI_S_COMMAND, *TELNET_CLI_S_COMMAND_PTR;

// for_register_cmd
typedef TELNET_CLI_S_COMMAND TELNET_S_CLICMD;

#define DECLARE_CLI_CMD_MACRO(name, child, Cbfunc, help, usermode) \
		DECLARE_TELNET_CLI_CMD_MACRO(name, child, Cbfunc, help, usermode)

#define DECLARE_CLI_CMD_MACRO_END()   DECLARE_TELNET_CLI_CMD_MACRO_END()
// end for_register_cmd

extern void TelnetTracePrint(const DEBUG_EN_LEVEL enLevel, char *pcpathname,
							const unsigned int u32Line, const char *fmt, ...);
extern int RegisterCliCommand(void *pstCliCmd);
extern void CliInit(int flag);
extern void CliDeInit(void);


#ifndef tcli_info
#define tcli_info(fmt...) TelnetTracePrint(DEBUG_EN_TRACE_INFO, __FILE__, __LINE__, fmt)
#endif

#ifndef tcli_warning
#define tcli_warning(fmt...) TelnetTracePrint(DEBUG_EN_TRACE_WARNING, __FILE__, __LINE__, fmt)
#endif

#ifndef tcli_error
#define tcli_error(fmt...) TelnetTracePrint(DEBUG_EN_TRACE_ERROR, __FILE__, __LINE__, fmt)
#endif


//always print,can not be control
#ifndef tcli_print
#define tcli_print(fmt...) TelnetTracePrint(DEBUG_EN_TRACE_NONE, __FILE__, __LINE__, fmt)
#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif
