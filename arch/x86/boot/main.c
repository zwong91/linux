// SPDX-License-Identifier: GPL-2.0-only
/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *   Copyright 2009 Intel Corporation; author H. Peter Anvin
 *
 * ----------------------------------------------------------------------- */

/*
 * Main module for the real-mode kernel code
 */
#include <linux/build_bug.h>

#include "boot.h"
#include "string.h"

struct boot_params boot_params __attribute__((aligned(16)));

struct port_io_ops pio_ops;

char *HEAP = _end;
char *heap_end = _end;		/* Default end of heap = no heap */

/*
 * Copy the header into the boot parameter block.  Since this
 * screws up the old-style command line protocol, adjust by
 * filling in the new-style command line pointer instead.
 如果是旧版本内核(0xA33F)，将hdr从第一个扇区的497个字节的位置复制到boot_params.hdr里面,  除此之外，boot_params别的成员对象都是干净的(未赋值)
         .globl  hdr
hdr:
setup_sects:    .byte 0                 // Filled in by build.c
root_flags:     .word ROOT_RDONLY
syssize:        .long 0                 // Filled in by build.c 
ram_size:       .word 0                 // Obsolete
vid_mode:       .word SVGA_MODE
root_dev:       .word 0                 // Filled in by build.c
boot_flag:      .word 0xAA55

*/

static void copy_boot_params(void)
{
	struct old_cmdline {
		u16 cl_magic;
		u16 cl_offset;
	};
	const struct old_cmdline * const oldcmd =
		absolute_pointer(OLD_CL_ADDRESS);

	BUILD_BUG_ON(sizeof(boot_params) != 4096);
	// 拷贝hdr结构到boot_params.hdr
	memcpy(&boot_params.hdr, &hdr, sizeof(hdr));

	// struct setup_header hdr;结构定义；引导加载程序应该只获取setup_header并放入将其放入干净的启动参数缓冲区
	if (!boot_params.hdr.cmd_line_ptr &&
	    oldcmd->cl_magic == OLD_CL_MAGIC) {
		/* Old-style command line protocol. */
		u16 cmdline_seg;

		/* Figure out if the command line falls in the region
		   of memory that an old kernel would have copied up
		   to 0x90000... */
		if (oldcmd->cl_offset < boot_params.hdr.setup_move_size)
			cmdline_seg = ds();
		else
			cmdline_seg = 0x9000;

		boot_params.hdr.cmd_line_ptr =
			(cmdline_seg << 4) + oldcmd->cl_offset;
	}
}

/*
 * Query the keyboard lock status as given by the BIOS, and
 * set the keyboard repeat rate to maximum.  Unclear why the latter
 * is done here; this might be possible to kill off as stale code.
 */
static void keyboard_init(void)
{
	struct biosregs ireg, oreg;
	initregs(&ireg);

	ireg.ah = 0x02;		/* Get keyboard status */
	intcall(0x16, &ireg, &oreg);
	boot_params.kbd_status = oreg.al;

	ireg.ax = 0x0305;	/* Set keyboard repeat rate */
	intcall(0x16, &ireg, NULL);
}

/*
 * Get Intel SpeedStep (IST) information.
 */
static void query_ist(void)
{
	struct biosregs ireg, oreg;

	/* Some older BIOSes apparently crash on this call, so filter
	   it from machines too old to have SpeedStep at all. */
	if (cpu.level < 6)
		return;

	initregs(&ireg);
	ireg.ax  = 0xe980;	 /* IST Support */
	ireg.edx = 0x47534943;	 /* Request value */
	intcall(0x15, &ireg, &oreg);

	boot_params.ist_info.signature  = oreg.eax;
	boot_params.ist_info.command    = oreg.ebx;
	boot_params.ist_info.event      = oreg.ecx;
	boot_params.ist_info.perf_level = oreg.edx;
}

/*
 * Tell the BIOS what CPU mode we intend to run in.
 */
static void set_bios_mode(void)
{
#ifdef CONFIG_X86_64
	struct biosregs ireg;

	initregs(&ireg);
	ireg.ax = 0xec00;
	ireg.bx = 2;
	intcall(0x15, &ireg, NULL);
#endif
}

static void init_heap(void)
{
	char *stack_end;

	if (boot_params.hdr.loadflags & CAN_USE_HEAP) {
		asm("leal %P1(%%esp),%0"
		    : "=r" (stack_end) : "i" (-STACK_SIZE));

		heap_end = (char *)
			((size_t)boot_params.hdr.heap_end_ptr + 0x200);
		if (heap_end > stack_end)
			heap_end = stack_end;
	} else {
		/* Boot protocol 2.00 only, no heap available */
		puts("WARNING: Ancient bootloader, some functionality "
		     "may be limited!\n");
	}
}

void main(void)
{
	init_default_io_ops();

	/* First, copy the boot header into the "zeropage" 将引导头拷贝到零页中 */
	copy_boot_params();

	/* Initialize the early-boot console 初始化控制台, 检测是否有串口可用，默认为ttyS0 */
	console_init();
	if (cmdline_find_option_bool("debug"))
		puts("early console in setup code\n");

	/* End of heap check 设置栈底指针*/
	init_heap();

	/* Make sure we have all the proper CPU support 检查所有cpu是否都能使用*/
	if (validate_cpu()) {
		puts("Unable to boot - please use a kernel appropriate "
		     "for your CPU.\n");
		die();
	}

	/* Tell the BIOS what CPU mode we intend to run in. 设置cpu运行模式(只对16、32位有效，设置为长模式)，64位, 此时还是实模式*/
	set_bios_mode();

	/* Detect memory layout 检测内存*/
	detect_memory();

	/* Set keyboard repeat rate (why?) and query the lock flags 设置键盘重复频率*/
	keyboard_init();

	/* Query Intel SpeedStep (IST) information 检查英特尔IST*/
	query_ist();

	/* Query APM information */
#if defined(CONFIG_APM) || defined(CONFIG_APM_MODULE)
	query_apm_bios();
#endif

	/* Query EDD information */
#if defined(CONFIG_EDD) || defined(CONFIG_EDD_MODULE)
	query_edd();
#endif

	/* Set the video mode 设置显式模式*/
	set_video();

	/* Do the last things and invoke protected mode 实模式跳转到保护模式*/
	go_to_protected_mode();
}
