/*----------------------------------------------------------------------------*/
/*
** EJTAG Command
**
** Date 	: 2013-07-02
** Author 	: wangshuke<anson.wang@foxmail.com>
** Mark		: Create For Windows Version
**----------------------------------------------------------------------------*/


#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "command.h"
#include "command_def.h"
#include "mips_ejtag.h"
#include "nand.h"

/*----------------------------------------------------------------------------*/
extern COMMAND commands[];

/*----------------------------------------------------------------------------*/


u32 jtag_break_valid = 0;
/*----------------------------------------------------------------------------*/
MIPS_EJTAG_STRU gst_mips_ejtag = {0}; 
MIPS_DEBUG_INFO gst_mips_debug = {0};

/*----------------------------------------------------------------------------*/
/* MIPS Reg Name */
s8 *reg_names[] = {	
	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra",
	"sr",	"lo",	"hi",	"bad",	"cause","pc",
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",
	"fsr",  "fir",  "fp",	"",
	"idx",	"rdm",	"elo0",	"el01",	"ctxt",	"pmsk",	"wired","",
	"bad",	"cnt1",	"ehi",	"cmp1",	"sr",	"cause","epc",	"prid",
	"cnfg","",	"cnt2",	"cmp2",	"cnt3",	"cmp3","", "debug",
	"depc",	"",	"",	"",	"",	"","",	"dsave", "ebase"
};

/*----------------------------------------------------------------------------*/
extern s32 load_file (char *filename, s32 offset, u32 *size);


extern s32 nand_init(void);
extern s32 nand_dma_read(u32 memaddr, u32 offset, s32 size);
extern s32 nand_write(u32 memaddr, u32 offset, u32 size);
extern s32 nand_erase(u32 offset);
extern s32 nand_dma_write(u32 memaddr, u32 offset, s32 size);

/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
s32 cmd_break(s8 *argv[])
{
	s8 *err_addr;
	u32 addr;

	if (NULL == argv[1])
	{
		if (argv[0][0] == EJTAG_COMMAND_SPECIAL_CHAR_NEG)
		{
			LOCK_MIPS_TAP();
			ejtag_del_all_bp();
			UNLOCK_MIPS_TAP();
		}
		else
			ejtag_show_bp_info();
		return CMD_PARAM_ERR;
	}

	addr = strtoul(argv[1], &err_addr, 16);
	if (NULL == err_addr)
	{
		printf("U input param Address[#x] error", argv[1]);
		return CMD_PARAM_ERR;
	}

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		if (argv[0][0] == EJTAG_COMMAND_SPECIAL_CHAR_NEG) 
			ejtag_del_bp(addr);
		else 
			ejtag_add_bp(addr);
	}
	UNLOCK_MIPS_TAP();
	
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_continue(char *argv[])
{
	LOCK_MIPS_TAP();
	if (USE_STEP == gst_mips_debug.step_mode)
	{
		ejtag_init_step_mode(NON_STEP);
		gst_mips_debug.step_mode = NON_STEP;
		printf("Do Continue, Exit Step Mode\n");
	}

	ejtag_release();
	UNLOCK_MIPS_TAP();
	
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_dbreak(char *argv[])
{
	
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_ddrinit(char *argv[])
{
	gst_mips_ejtag.cmd_run_state = CMD_DDRINIT;
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_ddr_init();
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_ddrtest(char *argv[])
{
	
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_ejtagboot(char *argv[])
{
	LOCK_MIPS_TAP();
	ejtag_boot();
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_ejtagbreak(char *argv[])
{
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		printf("Cpu Stop At pc[0x%08x]", 
							ejtag_get_reg(PC_REGNUM));
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_filecheck(char *argv[])
{
	
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_fill(char *argv[])
{
	s8 *addrp, *valp, *countp;
	u32 addr, val, count;

	if (NULL == argv[1] || NULL == argv[2] || NULL == argv[3])
	{
		printf("U should givea param: addr val count\n");
		return CMD_PARAM_ERR;
	}


	addr = strtoul(argv[1], &addrp, 16);
	val = strtoul(argv[2], &valp, 16);
	count = strtoul(argv[3], &countp, 16);
	if (NULL == addrp || NULL == valp || NULL == countp) 
	{
		printf("Input param error\n");
		return CMD_PARAM_ERR;
	}

	printf("addr[0x%08x] val[0x%08x] count[0x%08x]", addr, val, count);
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_fill_mem(addr, val, count);
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_help(s8 *argv[])
{
	u32 i;
	u32 printed = 0;

	for (i = 0; commands[i].name; i++) 
	{
		if (!argv[1] && !commands[i].alias) 
		{
			printf("%-10s- %s\n", commands[i].name, commands[i].usage);
			printed++;
		} 
		else if (argv[1] && strcmp(argv[1], commands[i].name) == 0) 
		{
			printf("%s:\n\t%s\n\t%s\n", commands[i].name, 
												 commands[i].format,
												 commands[i].usage);
			printed++;
		}
	}

	if (!printed) 
	{
		printf("No commands match '%s'.Possibilties are:\n", argv[1]);

		for (i = 0; commands[i].name; i++) 
		{
			if (printed == 6) 
			{
				printed = 0;
				printf("\n");
			}

			printf("%s\t", commands[i].name);
			printed++;
		}

		if (printed)
			printf("\n");
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_ibreak(char *argv[])
{
	printf("in func[cmd_ibreak]");
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_init(char *argv[])
{
	printf("in func[cmd_init]");
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_md(char *argv[])
{
	s8 *err_addr = NULL;
	s8 *err_count = NULL;
	u32 addr = 0;
	u32 count = 0;
	u32 data_size = 0;

	if ((NULL == argv[1]) || (NULL == argv[2]))
	{
		printf("You Should input reg_index and value\n");
		return CMD_PARAM_ERR;
	}
	addr = strtoul(argv[1], &err_addr, 16);
	count = strtoul(argv[2], &err_count, 16);
	if (NULL == err_addr || NULL == err_count) 
	{
		printf("Input param error\n");
		return CMD_PARAM_ERR;
	}
	data_size = cmd_get_data_size(argv[0]);
	if (0 >= data_size)
	{
		printf("No this cmd->%s", argv[0]);
		return CMD_PARAM_ERR;
	}

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_mem_display(addr, count, data_size);	
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_quit(s8 *argv[])
{
	WriteHistory Func;
	Func = (WriteHistory)GetProcAddress(hHistoryInst, "write_history");

	if (Func(EJTAT_COMMAND_HISTORY) < 0)
		printf("Error: command history save failed\n");

	gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	ejtag_sigint_exit(DONT_CARE);

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_run(char *argv[])
{
	printf("in func[cmd_run]");
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_read(char *argv[])
{
	u32 count = 0;
	s8 *err_addr = NULL;
	u32 addr = 0;
	u32 data_size = 0;

	if (NULL == argv[1])
	{
		printf("cmd_read U should give addr param!\n");
		return CMD_PARAM_ERR;
	}
	addr = strtoul(argv[1], &err_addr, 16);
	if (NULL == err_addr) 
	{
		printf("cmd_read Input param error\n");
		return CMD_PARAM_ERR;
	}
	data_size = cmd_get_data_size(argv[0]);
	if (0 >= data_size)
	{
		printf("No this cmd->%s", argv[0]);
		return CMD_PARAM_ERR;
	}
	
	if (NULL != argv[2])
	{
		count = strtoul(argv[2], &err_addr, 16);
		if (NULL == err_addr) 
		{
			printf("cmd_read Input param error\n");
			return CMD_PARAM_ERR;
		}
	}
	else
		count = data_size;

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_mem_read(addr, count, data_size);
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_reg(char *argv[])
{
	s8 *err_idx = NULL;
	s8 *err_val = NULL;
	u32 reg_idx = 0;
	u32 reg_val = 0;

	if ((NULL == argv[1]) || (NULL == argv[2]))
	{
		printf("You Should input reg_index and value\n");
		return CMD_PARAM_ERR;
	}
	reg_idx = strtoul(argv[1], &err_idx, 10);
	reg_val = strtoul(argv[2], &err_val, 16);
	if (NULL == err_idx || NULL == err_val) 
	{
		printf("Input param error\n");
		return CMD_PARAM_ERR;
	}

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_set_reg(reg_idx, reg_val);
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
/*	dump all regs
*/
s32 cmd_reginfo(char *argv[])
{
	u32 i  = 0;
	u32 ctrl_reg = 0;

	/* enter debug mode wait cpu pending */
	/* ejtag_init_wait_pending should wait sometimes */
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		show_mips_regs();
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_show(s8 *argv[])
{
	s8 *subcmd = argv[1];

	if (NULL == subcmd)
	{
		printf("U Should Give SubCmd: debug or reg\n");
		return CMD_PARAM_ERR;
	}

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		if (strncmp(subcmd, "debug", strlen("debug")) == 0) 
		{
			ejtag_show_cp0_debug();
		} 
		else if (strncmp(subcmd, "reg", strlen("reg")) == 0) 
		{
			ejtag_show_reg(argv);
		} 
		else 
		{
			printf(
					"Error: A valid sub-command MUST be given, %s!\n", argv[1]);
		}
	}

	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_singlestep(char *argv[])
{
	u32 i = 0;
	u8 step_num = 1;
	s8 *err_pos = NULL;

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (!jtag_break_valid)
	{
		goto exit;
	}
	if (EJTAG_COMMAND_SPECIAL_CHAR_NEG == argv[0][0])
	{
		ejtag_init_step_mode(NON_STEP);
		gst_mips_debug.step_mode = NON_STEP;
		printf("Exit Step Debug Mode PC[0x%08x]\n", 
													ejtag_get_reg(37));
		goto exit;
	}

	if (NON_STEP == gst_mips_debug.step_mode)
	{
		ejtag_init_step_mode(USE_STEP);
		gst_mips_debug.step_mode = USE_STEP;
		printf("Enter Step Debug Mode PC[0x%08x]\n", 
													ejtag_get_reg(37));
	}
	else
	{
		if (NULL != argv[1])
		{
			step_num = (u8)strtoul(argv[1], &err_pos, 10);
			if (NULL == err_pos)
			{
				printf("In Step Opt, input step num error\n");
				goto exit;
			}
		}
		printf("Run %d Step;", step_num);
		while (step_num-- > 0)
		{
			ejtag_release();
		}
		printf(" PC[0x%08x]\n", ejtag_get_reg(37));
	}

exit:
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_test(char *argv[])
{

	ejtag_test();

#if 0
	unsigned long addr = 0xa1000000, data, i;
	unsigned long len = 0x8000000;
	data = 0xfafafafa;
	for (i = addr; i < addr + len; i += 4) {
		mips_ejtag_praccWrite_w(i, i);
		data = mips_ejtag_praccRead_w(i);
		if (data != i) {
			printf("Error: data(%lx) addr(%lx)\n", data, i);
//			data = 0xfafafafa;
		} else {
			printf("addr(%lx) passed!\n", i);
		}
	}
#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_memload(char *argv[])
{
	u32 ret = SUCCESS;
	u32 dest_addr = 0xbfc00000;
	u32 init_pc_reg = 0xa0010000;
	s8 *err_pos = NULL;
	s8 *file_name = NULL;
	u32 size = 0;

	if (NULL == argv[1])
	{
		printf("Error, load file[NULL]\n");
		return LOAD_PARAM_ERR;
	}
	
	file_name = argv[1];
	if (argv[2])
	{
		dest_addr = strtoul(argv[2], &err_pos, 16);
		if (NULL == err_pos)
		{
			printf("Input addr error %s\n", err_pos);
			return LOAD_PARAM_ERR;
		}
	}

	if (argv[3])
	{
		init_pc_reg = strtoul(argv[3], &err_pos, 16);
		if (NULL == err_pos)
		{
			printf("Input init_pc_reg error %s\n", err_pos);
			if (ECLIPSE_MODE == gst_mips_debug.run_mode)
				return LOAD_MEM_PARAM_ERR;
		}
	}
	
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ret = load_file(file_name, dest_addr, &size);
		ejtag_set_reg(PC_REGNUM, init_pc_reg);
	}

	UNLOCK_MIPS_TAP();
	
	return ret;
}
/*----------------------------------------------------------------------------*/
s32 cmd_write(char *argv[])
{
	s8 *err_addr = NULL;
	s8 *err_data = NULL;
	u32 addr = 0;
	u32 data = 0;
	u32 data_size = 0;

	if ((NULL == argv[1]) || (NULL == argv[2]))
	{
		printf("You Should input addr and data value\n");
		return 0;
	}
	addr = strtoul(argv[1], &err_addr, 16);
	data = strtoul(argv[2], &err_data, 16);
	if (NULL == err_addr || NULL == err_data) 
	{
		printf("Input param error\n");
		return 0;
	}
	data_size = cmd_get_data_size(argv[0]);
	if (0 >= data_size)
	{
		printf("No this cmd->%s", argv[0]);
		return 0;
	}

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_mem_write(addr, data, data_size);
	}
	UNLOCK_MIPS_TAP();

	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_go(char *argv[])
{
	u32 addr = 0;
	s8 *err_addr = NULL;

	if (NULL == argv[1])
	{
		printf("You Should input addr \n");
		return 0;
	}
	addr = strtoul(argv[1], &err_addr, 16);
	if (NULL == err_addr) 
	{
		printf("Input param error\n");
		return 0;
	}
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();	
	if (jtag_break_valid)
	{
		ejtag_go_address(addr);	
	}
	UNLOCK_MIPS_TAP();
	
	return 0;
}
/*----------------------------------------------------------------------------*/
s32 cmd_nand(s8 *argv[])
{
	s32 size;
	s8 *subcmd;
    s8 *endp;
	u32 ret = SUCCESS;
	u32 offset = 0,memaddr;

	if (NULL == argv[1])
	{
		printf("U Should Give SubCmd: probe|read|write|erase|load\n");
		return LOAD_PARAM_ERR;
	}

	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (!jtag_break_valid)
	{
		goto UNLOCK;
	}
	if (!strcmp("blx_download", argv[0]))
		subcmd = "load";
	else
		subcmd = argv[1];

	if (!strcmp(subcmd, "probe"))
		nand_init();
	else if (!strcmp(subcmd, "read"))
	{
		if (NULL == argv[2] || NULL == argv[3] || NULL == argv[4])
		{
			printf("U Should Give Read param: addr offset size\n");
			goto UNLOCK;
		}
		memaddr = strtoul(argv[2], &endp, 16);
		offset = strtoul(argv[3], &endp, 16);
		size = strtoul(argv[4], &endp, 16);
		if (NULL == endp)
		{
			printf("U Should param: error\n");
			goto UNLOCK;
		}
		nand_dma_read(memaddr, offset, size);
	}
	else if (!strcmp(subcmd, "write"))
	{
		if (NULL == argv[2] || NULL == argv[3] || NULL == argv[4])
		{
			printf("U Should Give Write param: addr offset size\n");
			goto UNLOCK;
		}
		memaddr = strtoul(argv[2], &endp, 16);
		offset = strtoul(argv[3], &endp, 16);
		size = strtoul(argv[4], &endp, 16);
		if (NULL == endp)
		{
			printf("U Should param: error\n");
			goto UNLOCK;
		}
		nand_write(memaddr, offset, size);
	} 
	else if (!strcmp(subcmd, "erase"))
	{
		if (NULL == argv[2])
		{
			printf("U Should Give Write param: addr\n");
			goto UNLOCK;
		}
		memaddr = strtoul(argv[2], &endp, 16);
		if (NULL == endp)
		{
			printf("U Should param: error\n");
			goto UNLOCK;
		}
		nand_erase(offset);
	}
	else if (!strcmp(subcmd, "load"))
	{
		if (NULL == argv[2] || NULL == argv[3])
		{
			printf("U Should Give Write param: addr offset size\n");
			ret = LOAD_PARAM_ERR;
			goto UNLOCK;
		}
		offset = strtoul(argv[3], &endp, 16);
		if (NULL == endp)
		{
			printf("U Should param: error\n");
			ret = LOAD_PARAM_ERR;
			goto UNLOCK;
		}
		ret = load_file(argv[2], 0xa0000000, &size);
		if (SUCCESS != ret)
		{
			printf("download to mem failed\n");
			goto UNLOCK;
		}
		ret = nand_dma_write(0xa0000000, offset, size);
	}
	else
		printf("U Should Give SubCmd: probe|read|write|erase|load\n");

UNLOCK:
	UNLOCK_MIPS_TAP();

	return ret;
}
/*----------------------------------------------------------------------------*/
/* command list
** NOTE: currently, command node should be added alphabetically.
*/
COMMAND commands[] = 
{
	{
		"break", 
		cmd_break, 
		"[!]break <address>",
		"Insert/Remove breakpoint",
		0,
		0
	},
	{
		"continue", 
		cmd_continue, 
		"{continue [step] | c [step]}",
		"Resort from ejtag interrupt <step> time, step default to 1.",
		1,
		0
	},
	{
		"c", 
		cmd_continue, 
		"alias for continue",
		"Resort from ejtag interrupt <step> time, step default to 1.",
		1,
		1
	},
	{
		"dbreak", 
		cmd_dbreak, 
		"[!]dbreak <address>",
		"Insert/Remove ejtag hardware data breakpoint",
		0,
		0
	},
	{
		"ddrinit", 
		cmd_ddrinit, 
		"ddrinit",
		"ddr controller initialization procedure",
		0,
		0
	},
	{
		"ddrtest", 
		cmd_ddrtest, 
		"ddrtest",
		"ddr write and read",
		1,
		0
	},
	{
		"ejtagboot", 
		cmd_ejtagboot, 
		"ejtagboot",
		"Invoke a ejtagboot process",
		0,
		0
	},
	{
		"ejtagbreak", 
		cmd_ejtagbreak, 
		"ejtagbreak",
		"Invoke a ejtag interrupt exception",
		0,
		0
	},
	{
		"filecheck",
		cmd_filecheck,
		"filecheck <path> [destination]",
		"file check between <path> and [destination](default bfc00000) ",
		0,
		0
	},
	{
		"fill", 
		cmd_fill, 
		"fill <address> <value> <count>",
		"fill word to <address> repeatedly",
		0,
		0
	},
	{
		"help", 
		cmd_help, 
		"help [command]",
		"Display info for command[s]",
		0,
		0
	},
	{
		"?", 
		cmd_help, 
		"? [command]",
		"Display info for command[s]",
		0,
		0
	},
	{
		"ibreak", 
		cmd_ibreak, 
		"[!]ibreak <address>",
		"Insert/Remove ejtag hardware instruction breakpoint",
		0,
		0
	},
	{
		"init", 
		cmd_init, 
		"init",
		"Do EJTAG TAP initialization",
		0,
		0
	},
	{ 
		"md", 
		cmd_md, 
		"md[.<b | h | w>] <addr> [count]",
		"Memory display",
		1,
		0
	},
	{ 
		"quit", 
		cmd_quit, 
		"{quit | x}",
		"Exit from the program",
		0,
		0
	},
	{
		"run", 
		cmd_run, 
		"run <filepath> <arg>",
		"Load the binary into ram, and execute it.",
		0,
		0
	},
	{
		"read", 
		cmd_read, 
		"read[.<b | h | w>] <address> [count]",
		"Read content from the adddress",
		1,
		0
	},
	{ 
		"reg", 
		cmd_reg, 
		"reg <register-number> <register-value>",
		"set <register-value> to the register of <register-number>",
		0,
		0
	},
	{ 
		"reginfo", 
		cmd_reginfo, 
		"{reginfo | r}",
		"Print information for registers in mips core",
		1,
		0
	},
	{ 
		"r", 
		cmd_reginfo, 
		"alias for reginfo",
		"Print information for registers in mips core",
		1,
		1
	},
	{ 
		"show", 
		cmd_show, 
		"show <sub-command>",
		"Execute the sub-command, display ejtag internals",
		0,
		0
	},
	{ 
		"singlestep", 
		cmd_singlestep, 
		"{[!]singlestep | [!]s}",
		"Enter And Do Steps Debug",
		1,
		0
	},
	{ 
		"s", 
		cmd_singlestep, 
		"alias for singlestep",
		"Enter And Do Steps Debug",
		1,
		1
	},
	{ 
		"test", 
		cmd_test, 
		"{[!]singlestep | [!]s}",
		"Set/Unset the core work on single step mode",
		0,
		0
	},
	{
		"x",
		cmd_quit,
		"alias for quit",
		"Exit from the program",
		0,
		1
	},
	{
		"memload",
		cmd_memload,
		"ubootload <path> [destination]",
		"Load uboot to [destination](default bfc00000) through JTAG cable",
		0,
		0
	},
	{ 
		"write", 
		cmd_write, 
		"write[.<b | h | w>] <address> <data>",
		"Write <data> to <address> with certain access way",
		1,
		0
	},

	{ 
		"go", 
		cmd_go, 
		"go <address> ",
		"go <address>",
		1,
		0
	},
	{    
	    "nand",
	    cmd_nand,
		"nand <probe|read|write|erase|load > <memaddr> <offset> <size> ",
		"nand program",
		1,
		0
	},
	{  //end of the command list
		(char *)NULL, 
		(rl_icppfunc_t *)NULL, 
		(char *)NULL,
		(char *)NULL,
		0,
		0
	}
};

/*----------------------------------------------------------------------------*/
/* Generator function for command completion.  STATE lets us know whether
** to start from scratch; without any state (i.e. STATE == 0), then we
** start at the top of the list. 
*/
char *command_generator(const char *text, int state)
{
	static int list_index, len;
	char *name;

	/* If this is a new word to complete, initialize now.  This includes
	saving the length of TEXT for efficiency, and initializing the index
	variable to 0. */
	if (!state) {
		list_index = 0;
		len = strlen (text);
	}

	//printf("text(%s) state(%d) list_index(%d)\n", text, state, list_index);
	/* Return the next name which partially matches from the command list. */
	while (name = commands[list_index].name) {
		list_index++;

		if (strncmp (name, text, len) == 0)
		{
			s8 *r = malloc(strlen(name) + 1);;
			strcpy(r, name);
			return r;
		}
	}

	/* If no names matched, then return NULL. */
	return NULL;
}

/*----------------------------------------------------------------------------*/
/* completion function
*/
char **fileman_completion(const char *text, int start, int end)
{
	char **matches;
	int i = 0;

	matches = (char **)NULL;

	/* If this word is at the start of the line, then it is a command
	** to complete.  Otherwise it is the name of a file in the current
	** directory. 
	*/
	if (start == 0)
	{
		rlCompletionMatches RlCompletionMatches;
		RlCompletionMatches = (rlCompletionMatches)GetProcAddress(hReadLineInst, "rl_completion_matches");
		matches = RlCompletionMatches(text, command_generator);
	}

	return (matches);
}

/*----------------------------------------------------------------------------*/
void init_readline()
{
	rl_readline_name = "mips_ejtag";
	rl_attempted_completion_function = fileman_completion;

	return;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Strip whitespace from the start and end of STRING.  
** Return a pointer into STRING. 
*/
char *strip_white(char *string)
{
	register char *s, *t;

	for (s = string; whitespace(*s); s++)
		;

	if (*s == 0)
		return s;

	t = s + strlen(s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';

	return s;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/



//.
/*----------------------------------------------------------------------------*/
/* Name is composed of A[.B], only A is used to find command
*/
COMMAND *find_command(char *name)
{
	register int i;
	char nbuf[32] = {0};
	char *dotptr = NULL;
	int isNEG = 0;

	if (name[0] == EJTAG_COMMAND_SPECIAL_CHAR_NEG) 
		isNEG = 1;	

	if (isNEG) 
		strncpy(nbuf, name + 1, sizeof(nbuf));
	else 
		strncpy(nbuf, name, sizeof(nbuf));

	dotptr = strstr(nbuf, ".");
	if (dotptr) {
		*dotptr = '\0';
	}

	for (i = 0; commands[i].name; i++) 
	{
		if (strcmp(nbuf, commands[i].name) == 0)
			return &commands[i];
	}

	return NULL;
}

/*----------------------------------------------------------------------------*/
/* find command name; instead " " of '\0'
*/
COMMAND *find_command_from_line(s8 *line)
{
	s8 cmd_buf[64] = {0};
	s8 *space_pos = NULL;

	strncpy(cmd_buf, line, sizeof(cmd_buf));
	space_pos = strstr(cmd_buf, " ");
	if (space_pos) {
		*space_pos = '\0';
	}

	return find_command(cmd_buf);
}

/*----------------------------------------------------------------------------*/
#define MAX_EJTAG_ARGUMENT_NUM 5
static s8 *ejtag_args[MAX_EJTAG_ARGUMENT_NUM] = {NULL};

/*----------------------------------------------------------------------------*/
s32 execute_line(char *line)
{
	register s32 i;
	COMMAND *command;
	s8 *word;
	s32 ejtag_args_index = 0;
	s32 line_len = 0;
	s32 ret = 0;

	for (i = 0; i < MAX_EJTAG_ARGUMENT_NUM; i++) {
		ejtag_args[i] = NULL;
	}

	/* Isolate the command word. */
	i = 0;
	line_len = strlen(line);

	assert(line[0] != 0);
	while (i < line_len) {
		//1, skip all space word
		while (line[i] && whitespace(line[i])) {
			line[i] = 0;
			i++;
		}

		//2, record a word at the beginning of the non-whitespace word
		if (line[i] && !whitespace(line[i])) {
			word = &line[i];
			ejtag_args[ejtag_args_index++] = word;
			i++;
		}

		//and skip all non-space word
		while (line[i] && !whitespace(line[i]))
			i++;
	}

	command = find_command(ejtag_args[0]);

	if (!command) {
		fprintf (stderr, "%s: No such command for ejtag.\n", ejtag_args[0]);
		return -1;
	}

	/* Call the function. */
	gst_mips_ejtag.cmd_run_state = CMD_RUN;
	ret = ((*(command->func))(ejtag_args));
	gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	return ret;
}
/*----------------------------------------------------------------------------*/


