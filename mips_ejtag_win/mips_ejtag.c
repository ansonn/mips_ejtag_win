/*----------------------------------------------------------------------------*/
/*
** MIPS EJTAG Operation
**
** Date 	: 2013-07-02
** Author 	: wangshuke<anson.wang@foxmail.com>
** Mark		: Create For Windows Version
**----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>

#include "command.h"
#include "command_def.h"
#include "mips_ejtag.h"

#define JLINKID  0x13660101
#define EP1      0x81   //in
#define EP2      0x2    //out
// Device vendor and product id.
#define MY_VID 0x1366
#define MY_PID 0x0101
// Device configuration and interface id.
#define MY_CONFIG 1
#define MY_INTF 0
extern MIPS_EJTAG_STRU gst_mips_ejtag;

u32 ejtag_get_idcode();
/*----------------------------------------------------------------------------*/
struct usb_dev_handle *usb_dev = NULL;


/*----------------------------------------------------------------------------*/
void ejtag_usb_exit(void)
{
	usb_close(usb_dev);
}
/*----------------------------------------------------------------------------*/
/* USB read & write 
*/
int ejtag_usb_write(unsigned char *buf, int len)
{
	int ac_len;

	ac_len = usb_bulk_write(usb_dev, EP2, buf, len, 1000); //100 ms 
	if (ac_len < 0)
	{
		printf("error writing:\n%s\n", usb_strerror());
	}
	return ac_len;
}
int ejtag_usb_read(unsigned char *buf, int len)
{
	int ac_len;

	memset(buf, 0x00, len);
	ac_len = usb_bulk_read(usb_dev, EP1, buf, len, 1000); //100 ms 
	if (ac_len < 0)
	{
		printf("error reading:\n%s\n", usb_strerror());
	}
	return ac_len;

}

/*----------------------------------------------------------------------------*/
/* Do Ejtag TAP instruction
*/
void tap_do_instruction(EJTAG_INSTRUCTION instr)
{
	unsigned char cmd[8] = { 0 };

	/* CMD, no response */
	cmd[0] = EMU_CMD_HW_JTAG_WRITE;
	/* input 16bit,  */
	cmd[2] = 16; // 4 + 5 + 2 --->patch 5

	cmd[4] = 3 ; //tms 1 1 0 0  + 0 0 0 0 1 + 1 0 --->patch 0 0 0 0 0
	cmd[5] = 3 ; //tms

	cmd[6] = (instr << 4) & 0xF0 ;
	cmd[7] = ((instr >> 4) & 0x01) | 2;

	ejtag_usb_write(cmd, 8);

	return;
}

/*----------------------------------------------------------------------------*/
/* Get Ejtag TAP reg
*/
u32 tap_wr_data(u32 data)
{
	u32 read_val = 0;
	unsigned char cmd[16] = { 0 }; //bits = 8 + 32 + 2 = 6bytes * 2 + 4 = 16


	//ejtag_usb_read(gst_mips_ejtag.read_buf, 4);

	cmd[0] = EMU_CMD_HW_JTAG3;
	cmd[2] = 48; //8 + 32 + 2; -->patch 6bits
	
	cmd[4] = 0x20 ; //tms 0 0 0 0 0 1 0 0 + 0....1 + 1 0 -->patch 0 0 0 0 0 0
	cmd[5] = 0;
	cmd[6] = 0;
	cmd[7] = 0;
	cmd[8] = 0x80;
	cmd[9] = 1;

	//tdi
	cmd[10] = 0;
	cmd[11] = data & 0xff;
	cmd[12] = (data >> 8 ) & 0xff;
	cmd[13] = (data >> 16) & 0xff;
	cmd[14] = (data >> 24) & 0xff;
	cmd[15] = 0;
	
	ejtag_usb_write(cmd, 16);
	ejtag_usb_read(gst_mips_ejtag.read_buf, 6);
	read_val = *(unsigned int * )&gst_mips_ejtag.read_buf[1];
	ejtag_usb_read(gst_mips_ejtag.read_buf, 1);

	return read_val;
}

/*----------------------------------------------------------------------------*/
void do_usb_cmd(u8 cmd)
{
	u8 cmd_buff[10] = { 0 };

	cmd_buff[0]= cmd;
	ejtag_usb_write(cmd_buff, 1);

	return;
}

/*----------------------------------------------------------------------------*/
/* Get some information
** 
*/
int get_ejtag_info(void)
{
	int size = 0;

	/* get cap */
    do_usb_cmd(EMU_CMD_GET_CAPS);                                                                                                     
	Sleep(1);
	ejtag_usb_read(gst_mips_ejtag.read_buf, 4);      
    printf("EJTAG cap		:0x%08x\n", 
						*(u32 *)gst_mips_ejtag.read_buf);

	/* get version */
	do_usb_cmd(EMU_CMD_GET_HW_VERSION);                                                                                                    
	Sleep(1);
   	ejtag_usb_read(gst_mips_ejtag.read_buf, 4);      
    printf("EJTAG Version		:0x%08x\n", 
						*(u32 *)gst_mips_ejtag.read_buf);


	/* get version */
	do_usb_cmd(EMU_CMD_VERSION);                                                                                                    
	Sleep(1);
  	ejtag_usb_read(gst_mips_ejtag.read_buf, 2);
	size = *(u16 *)gst_mips_ejtag.read_buf;
    ejtag_usb_read(gst_mips_ejtag.read_buf, size);
	printf("EJTAG Info		:%s\n", gst_mips_ejtag.read_buf);

	do_usb_cmd(EMU_CMD_GET_STATE);
	Sleep(1);
	ejtag_usb_read(gst_mips_ejtag.read_buf, 8);
	printf("EJTAG state:");
	printf("		:vol[%04x]", *(u16 *)gst_mips_ejtag.read_buf);
	printf("\n			:tck[%02x]", (u32)gst_mips_ejtag.read_buf[2]);
	printf("\n			:tdi[%02x]", (u32)gst_mips_ejtag.read_buf[3]);
	printf("\n			:tdo[%02x]", (u32)gst_mips_ejtag.read_buf[4]);
	printf("\n			:tms[%02x]", (u32)gst_mips_ejtag.read_buf[5]);
	printf("\n			:tres[%02x]", (u32)gst_mips_ejtag.read_buf[6]);
	printf("\n			:trst[%02x]\n", (u32)gst_mips_ejtag.read_buf[7]);


	do_usb_cmd(EMU_CMD_HW_CLOCK);
    Sleep(1);
    ejtag_usb_read(gst_mips_ejtag.read_buf, EJTAG_BUFF_SIZE * 2);      

	return 0;
}


/*----------------------------------------------------------------------------*/
/* 	from TAP Controller State Diagram, know 
**	the TAP SM reset in Test-Logic-Reset state
*/
void ejtag_reset(void)
{
	u8 cmd[10] = { 0 };                                                                                                     

 	cmd[0] = EMU_CMD_HW_JTAG_WRITE;
	cmd[1] = 0;					//dummy                                                                                                   
    cmd[2] = 8;					//8 bits
	cmd[3] = 0;					//...
   	cmd[4] = 0x7F;				//TMS check to reset state
	cmd[5] = 0x0; 				//TDI                                                                                                              
                                                                                                                               
    ejtag_usb_write(cmd, 6);  
}

/*----------------------------------------------------------------------------*/
void ejtag_set_speed(int n)
{
	u8 cmd[10] = { 0 };

	cmd[0] = EMU_CMD_SET_SPEED;
	cmd[1] = n & 0xFF;
	cmd[2] = (n >> 8) & 0xFF; //n K bits
	
	ejtag_usb_write(cmd,3); 	
}

/*----------------------------------------------------------------------------*/
/* 打开指定设备 */
usb_dev_handle *open_dev(void)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor == MY_VID
				&& dev->descriptor.idProduct == MY_PID)
			{
				return usb_open(dev);
			}
		}
	}
	return NULL;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_init()
{

	memset(&gst_mips_ejtag, 0x00, sizeof(MIPS_EJTAG_STRU));

	/* windows使用的libusb库的方法! */
	usb_init();				/* initialize the library */
	usb_find_busses();		/* find all busses */
	usb_find_devices();		/* find all connected devices */

	usb_dev = open_dev();
	if (NULL == usb_dev)
	{
		printf("Not found jtag v8\n");
		return NO_FOUND_JTAG;
	}

	if (usb_set_configuration(usb_dev, MY_CONFIG) < 0)
	{
		printf("Eerror Setting Config #%d: %s\n", MY_CONFIG, usb_strerror());
		usb_close(usb_dev);
		return SET_LIBUSB_FAILED;
	}
	if (usb_claim_interface(usb_dev, 0) < 0)
	{
		printf("Error Claiming Interface #%d:\n%s\n", MY_INTF, usb_strerror());
		usb_close(usb_dev);
		return SET_LIBUSB_FAILED;
	}

	/* TRST reset; TRST low level, SM force in Test-Logic-Reset */
	do_usb_cmd(EMU_CMD_HW_TRST0);
	Sleep(1);
	do_usb_cmd(EMU_CMD_HW_TRST1);

	get_ejtag_info();
	Sleep(1);

	ejtag_set_speed(10000);
	Sleep(1);

	/* init SM in Test-Logic-Reset */
	ejtag_reset();
	Sleep(1);

	printf("EJTAG IDCODE[0x%08x]\n", ejtag_get_idcode());

	return 0;
}


/*----------------------------------------------------------------------------*/




/*----------------------------------------------------------------------------*/
/* mips ejtag debug mode program
*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Active Ejtag Debug Mode; 
** Write Ctrl_Reg bit[PRACC | PROBEN | SETDEV | JTAGBRK]
*/
u32 ejtag_init_wait_pending()
{
	u32 read_num = 32;
	u32 ctrl_reg = 0;
	
	if (gst_mips_ejtag.ejtag_mode == DEBUG)
		return 0;

	/* select ejtag ctrl reg */
	tap_do_instruction(EJG_CONTROL);
	/* write ejtag ctrl reg */
	tap_wr_data(PRACC | PROBEN | PROBTRAP | JTAGBRK);

	/* read ctrol reg, check valid */
	while (0 < --read_num)
	{
		ctrl_reg = tap_wr_data(PRACC | PROBEN | PROBTRAP | JTAGBRK);
		if (ctrl_reg & PRACC)
		{
			gst_mips_ejtag.ejtag_mode = DEBUG;
			return ctrl_reg;
		}
		
	}
	printf("Trg JtagBreak 32 Times; ctrl_reg[%#x]\n", 
												ctrl_reg);
	return ctrl_reg;
}
/*----------------------------------------------------------------------------*/
/* INIT Mips Ejtag Buffer
*/
void ejtag_init_buffer(void)
{
	memset(gst_mips_ejtag.tmsbuf, 0x00, EJTAG_BUFF_SIZE);
	memset(gst_mips_ejtag.tdibuf, 0x00, EJTAG_BUFF_SIZE);
	gst_mips_ejtag.pos = 0;
	gst_mips_ejtag.nbits = 0;
}

/*----------------------------------------------------------------------------*/
void ejtag_send_buffer(USB_RET_FLAG ret_flag)
{	
	if (EJG_RET == ret_flag)
		gst_mips_ejtag.send_buf[0] = EMU_CMD_HW_JTAG3;
	else
		gst_mips_ejtag.send_buf[0] = EMU_CMD_HW_JTAG_WRITE;
	
	gst_mips_ejtag.send_buf[2] = gst_mips_ejtag.nbits & 0xff;
	gst_mips_ejtag.send_buf[3] = (gst_mips_ejtag.nbits >> 8) & 0xff;

	memcpy(&gst_mips_ejtag.send_buf[4], gst_mips_ejtag.tmsbuf, 
										gst_mips_ejtag.pos);
	memcpy(&gst_mips_ejtag.send_buf[4] + gst_mips_ejtag.pos, 
						 gst_mips_ejtag.tdibuf, gst_mips_ejtag.pos);

	ejtag_usb_write(gst_mips_ejtag.send_buf, gst_mips_ejtag.pos * 2 + 4);
}

/*----------------------------------------------------------------------------*/
u8 *ejtag_read_buffer()
{
	ejtag_usb_read(gst_mips_ejtag.read_buf, gst_mips_ejtag.pos);

	return (u8 *)gst_mips_ejtag.read_buf;
}

/*----------------------------------------------------------------------------*/
int ejtag_buffer_inuse_size(void)
{
	return gst_mips_ejtag.pos;
}

/*----------------------------------------------------------------------------*/
void ejtag_add_instruction_buff(EJTAG_INSTRUCTION instruction)
{
	/* 128: protect */
	if (gst_mips_ejtag.pos >= EJTAG_BUFF_SIZE - 128)
	{
		printf("\ninstruction buff out of mem");
		return;
	}

	gst_mips_ejtag.nbits += 16;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos] = 3;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos + 1] = 3;

	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos] = (instruction << 4) & 0xF0 ;  
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos + 1] = 
								((instruction >> 4) & 0x1) | 2 ;

	gst_mips_ejtag.pos += 2;
}
/*----------------------------------------------------------------------------*/
void ejtag_add_data_buff(u32 data)
{
	/* 128: protect */
	if (gst_mips_ejtag.pos >= EJTAG_BUFF_SIZE - 128)
	{
		printf("\ndata buff out of mem");
		return;
	}
	gst_mips_ejtag.nbits += 48 ;

	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos] = 0x20;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos + 1] = 0;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos + 2] = 0;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos + 3] = 0;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos + 4] = 0x80;
	gst_mips_ejtag.tmsbuf[gst_mips_ejtag.pos + 5] = 0x1;
	 
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos] = 0;
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos + 1] = data & 0xff;
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos + 2] = (data >>8) & 0xff;
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos + 3] = (data >>16) & 0xff;
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos + 4] = (data >>24) & 0xff;
	gst_mips_ejtag.tdibuf[gst_mips_ejtag.pos + 5] = 0;

	gst_mips_ejtag.pos += 6 ;
}
/*----------------------------------------------------------------------------*/
void ejtag_add_assembly_buffer(u32 code)
{
	//switch ejtag data reg				16bit
	ejtag_add_instruction_buff(EJG_DATA);	
	//add assemble						48bit
	ejtag_add_data_buff(code);
	//run assemble						16bit
	ejtag_add_instruction_buff(EJG_CONTROL);		
	//add assemble						48bit
	ejtag_add_data_buff(PROBEN | PROBTRAP);				
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
u32 ejtag_get_idcode(void)
{
	u32 impcode = 0;
	u32 ejtag_version = 0;

	tap_do_instruction(EJG_IDCODE);
	impcode = tap_wr_data(DONT_CARE);


/* get ejtag version */
	ejtag_version = ((impcode >> 29) & 0x07);
	switch (ejtag_version)
	{
		case 0:
			printf("\nEJTAG: Version 1 or 2.0 Detected\n");
			break;
		case 1:
			printf("\nEJTAG: Version 2.5 Detected\n");
			break;
		case 2:
			printf("\nEJTAG: Version 2.6 Detected\n");
			break;
		case 3:
			printf("\nEJTAG: Version 3.1 Detected\n");
			break;
		default:
			printf("\nEJTAG: Unknown Version Detected\n");
			break;
	}
	printf("EJTAG: features:%s%s%s%s%s%s%s\n",
					impcode & EJTAG_IMP_R3K ? " R3k" : " R4k",
					impcode & EJTAG_IMP_DINT ? " DINT" : "",
					impcode & (1 << 22) ? " ASID_8" : "",
					impcode & (1 << 21) ? " ASID_6" : "",
					impcode & EJTAG_IMP_MIPS16 ? " MIPS16" : "",
					impcode & EJTAG_IMP_NODMA ? " noDMA" : " DMA",
					impcode & EJTAG_DCR_MIPS64  ? " MIPS64" : " MIPS32");

	if ((impcode & EJTAG_IMP_NODMA) == 0)
		printf("EJTAG: DMA Access Mode Support Enabled\n");

	return impcode;
}
/*----------------------------------------------------------------------------*/



