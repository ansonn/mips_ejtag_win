/*----------------------------------------------------------------------------*/
/*
** MIPS EJTAG main
**
** Date 	: 2013-07-02
** Author 	: wangshuke<anson.wang@foxmail.com>
** Mark		: Create For Windows Version
**----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <pthread.h>

#include "command_def.h"
#include "command.h"
#include "mips_ejtag.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
extern s32 ejtag_init();
extern void init_readline();
extern void ejtag_usb_exit(void);
extern s32 cmd_continue(char *argv[]);
void do_usb_cmd(u8 cmd);
s32 cmd_memload(char *argv[]);
s32 cmd_nand(s8 *argv[]);


HINSTANCE hReadLineInst;
HINSTANCE hHistoryInst;

/*----------------------------------------------------------------------------*/
void init_pthread_mutex(void)
{
	/* windows互斥；Linux方法可考虑后面使用宏开关区分 */
	gst_mips_ejtag.tap_mutex = CreateMutex(NULL, FALSE, "tap_mutex");
	gst_mips_debug.list_mutex = CreateMutex(NULL, FALSE, "list_mutex");
}

/*----------------------------------------------------------------------------*/
void destroy_pthread_mutex(void)
{
	/* win 方法 */
	CloseHandle(gst_mips_ejtag.tap_mutex);
	CloseHandle(gst_mips_debug.list_mutex);
}

/*----------------------------------------------------------------------------*/
void ejtag_exit(u32 exit_code)
{
	printf("EJTAG exit_code[%d]\n", exit_code);
	FreeLibrary(hReadLineInst);//释放Dll函数
	FreeLibrary(hHistoryInst);//释放Dll函数
	exit(exit_code);
}

/*----------------------------------------------------------------------------*/
void ejtag_sigint_exit(s32 signo)
{
	if (CMD_RUN == gst_mips_ejtag.cmd_run_state)
	{
		printf("\n\nHave Command Run Now, Dont Ctrl+C!" 
						" Wait Command Done\n\n");
		return;
	}
	else if (CMD_DDRINIT == gst_mips_ejtag.cmd_run_state)
	{
		ejtag_exit(0);
	}
	cmd_continue(NULL);
	do_usb_cmd(EMU_CMD_HW_TRST0);
	Sleep(1);
	do_usb_cmd(EMU_CMD_HW_TRST1);
	Sleep(1);
	ejtag_usb_exit();
	destroy_pthread_mutex();
	ejtag_exit(0);
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_download(s8 argc, s8 *argv[])
{
	s32 ret = SUCCESS;

	if (4 > argc)
	{
		printf("blx_download param error\n");
		return LOAD_PARAM_ERR;
	}

	if (!strcmp("mem", argv[1]))
	{
		if (5 != argc)
		{
			printf("blx_download mem param error\n");
			return LOAD_MEM_PARAM_ERR;
		}
	
		gst_mips_ejtag.cmd_run_state = CMD_RUN;
		ret = cmd_memload(argv + 1);
		gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	}
	else if (!strcmp("nand", argv[1]))
	{
		gst_mips_ejtag.cmd_run_state = CMD_RUN;
		ret = cmd_nand(argv);
		gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	}
	else if (!strcmp("nor", argv[1]))
	{
		printf("blx_download nor dont support\n");
		gst_mips_ejtag.cmd_run_state = CMD_RUN;
		ret = OPT_NO_SUPPORT;
		gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	}
	else
		ret = LOAD_PARAM_ERR;
	
	return ret;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_debug(s8 argc, s8 *argv[])
{
	u32 ret = 0;
	s8 *err_pos = NULL;

	if (2 != argc)
	{
		printf("blx_debug param error\n");
		return DEBUG_PARAM_ERR;
	}
	else
	{
		printf("Have No This Function\n");
		return DEBUG_PARAM_ERR;	
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_command(s8 argc, s8 *argv[])
{
	s8 *line = NULL;
	s8 *s = NULL;
	HIST_ENTRY *hent = NULL;
    s32 history_offset;
    char exe_line[128] = { 0 };

	WhereHistory Func_where_history;
	ReadLine Func_readline;
	HistoryGet Func_history_get;
	AddHistory Func_AddHistory;
	XFree X_Free;

	Func_readline		= (ReadLine)GetProcAddress(hReadLineInst, "readline");
	Func_where_history	= (WhereHistory)GetProcAddress(hReadLineInst, "where_history");
	Func_history_get	= (HistoryGet)GetProcAddress(hReadLineInst, "history_get");
	Func_AddHistory		= (AddHistory)GetProcAddress(hReadLineInst, "add_history");
	X_Free				= (XFree)GetProcAddress(hReadLineInst, "xfree");

	init_readline();
	while (1) 
	{
		line = Func_readline("mips_ejtag: ");
		if (!line) 
		{
			/* ctrl + D */
			ejtag_release();
			printf("\n");
			break;
		} 
		else if (line[0] == '\0') 
		{
			history_offset = Func_where_history();
			hent = Func_history_get(history_offset);
			if (hent) 
			{
				//printf("\n hent->line = %s", hent->line);
				COMMAND *cmd = find_command_from_line(hent->line);
				if (cmd && cmd->repeatable) 
				{
					memset(exe_line, 0, sizeof(exe_line));
					strncpy(exe_line, hent->line, sizeof(exe_line));
					s = exe_line;
					goto REPEATCMD;
				}
			}
			else
			{
				//printf("\n hent == NULL");
			}
		}

		/* remove both end whitespace */
		s = strip_white(line);
		
REPEATCMD:
		if (*s) 
		{
			Func_AddHistory(s);
			execute_line(s);
		} 

		X_Free(line);
	}
	
	return SUCCESS;
}

/*----------------------------------------------------------------------------*/
void ejtag_usage(void)
{
	printf("EJTAG Usage:\n");
	printf("	./ejtag blx_download mem/nand/nor file address:\n");
	printf("	./ejtag blx_debug gdb_port:\n");
	printf("	./ejtag blx_gdb gdb_port:\n");
	printf("	./ejtag blx_command:\n");
	printf("Notes:\n");
	printf("	blx_download\t: do download file\n");
	printf("	blx_debug\t: start gdb server only\n");
	printf("	blx_gdb\t\t: start gdb server and command line\n");
	printf("	blx_command\t: start command func only\n");
	printf("\n");

}

/*----------------------------------------------------------------------------*/
s32 main(s8 argc, s8 *argv[])
{
	s32 ret = 0;

	hReadLineInst	= LoadLibrary("readline5.dll");//动态加载Dll;
	hHistoryInst	= LoadLibrary("history5.dll");//动态加载Dll;


	printf("Welcom MIPS EJTAG!\n");
	init_pthread_mutex();
	ret = ejtag_init();
	if (ret) 
		ejtag_exit(ret);
	signal(SIGINT, ejtag_sigint_exit);

	if (2 > argc)
	{
		gst_mips_debug.run_mode = EJTAG_MODE;
		printf("\nblx_command! enter ejtag mode\n");
		printf("	U can use command only\n");
		ret = ejtag_blx_command(argc - 1, argv + 1);
		ejtag_exit(ret);
	}

	if (!strcmp("blx_download", argv[1]))
	{
		gst_mips_debug.run_mode = ECLIPSE_MODE;
		printf("\nblx_download! enter eclipse mode\n");
		ret = ejtag_blx_download(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else if (!strcmp("blx_debug", argv[1]))
	{
		gst_mips_debug.run_mode = ECLIPSE_MODE;
		printf("\nblx_debug! enter eclipse mode\n");
		ret = ejtag_blx_debug(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else if (!strcmp("blx_gdb", argv[1]))
	{
		gst_mips_debug.run_mode = EJTAG_MODE;
		printf("\nblx_gdb! enter ejtag mode\n");
		printf("	Have No This Function\n");

		ejtag_exit(ret);
	}
	else if (!strcmp("blx_command", argv[1]))
	{
		gst_mips_debug.run_mode = EJTAG_MODE;
		printf("\nblx_command! enter ejtag mode\n");
		printf("	U can use command only\n");
		ret = ejtag_blx_command(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else
	{
		ejtag_usage();
		ejtag_exit(CMD_PARAM_ERR);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/










