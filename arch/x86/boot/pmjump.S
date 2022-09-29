/* SPDX-License-Identifier: GPL-2.0-only */
/* ----------------------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *
 * ----------------------------------------------------------------------- */

/*
 * The actual transition into protected mode
SYM_FUNC_START_NOALIGN(protected_mode_jump)
	||
	\/
SYM_START(protected_mode_jump, (.globl protected_mode_jump), SYM_A_NONE)  .globl protected_mode_jump声明为外部程序可访问的标签(函数)
	||
	\/
SYM_ENTRY(protected_mode_jump,  (.globl protected_mode_jump), )
	||
	\/
.globl protected_mode_jump


SYM_FUNC_END(protected_mode_jump)
	||
	\/
SYM_END(protected_mode_jump, 2)
	||
	\/
.type protected_mode_jump 2 ; //标记protected_mode_jump为函数名
.size protected_mode_jump .-protected_mode_jump //得到protected_mode_jump函数占用字节大小

 */

#include <asm/boot.h>
#include <asm/processor-flags.h>
#include <asm/segment.h>
#include <linux/linkage.h>

	.text
	.code16

/*
 * void protected_mode_jump(u32 entrypoint, u32 bootparams);
 */
SYM_FUNC_START_NOALIGN(protected_mode_jump)
	movl	%edx, %esi		# Pointer to boot_params table boot_params结构对象放入源索引寄存器

	xorl	%ebx, %ebx		# 清空基地址寄存器
	movw	%cs, %bx		# 代码段寄存器的地址放入基地址寄存器(16位)，然后向左移动4位并将其添加到2f的内存地址
	shll	$4, %ebx
	addl	%ebx, 2f		# 转换为32位模式后要跳转的物理地址，跳转到1
	jmp	1f			# Short jump to serialize on 386/486
1:

	movw	$__BOOT_DS, %cx		# 数据段放入cx、任务状态段放入di
	movw	$__BOOT_TSS, %di

	movl	%cr0, %edx			# 设置保护允许位PE(Protedted Enable)，用于启动保护模式
	orb	$X86_CR0_PE, %dl		# Protected mode
	movl	%edx, %cr0

	# Transition to 32-bit mode
	.byte	0x66, 0xea		# ljmpl opcode	跳转到32位保护模式(.Lin_pm32)，0x66允许混合16位和32位代码，0xea是跳转操作码
2:	.long	.Lin_pm32		# offset
	.word	__BOOT_CS		# segment
SYM_FUNC_END(protected_mode_jump)

	.code32
	.section ".text32","ax"		# 使用32位编码，设置.text32段(允许执行)
SYM_FUNC_START_LOCAL_NOALIGN(.Lin_pm32)	 # 定义函数.Lin_pm32
	# Set up data segments for flat 32-bit mode  设置相关数据段
	movl	%ecx, %ds
	movl	%ecx, %es
	movl	%ecx, %fs
	movl	%ecx, %gs
	movl	%ecx, %ss
	# The 32-bit code sets up its own stack, but this way we do have
	# a valid stack if some debugging hack wants to use it. 设置用于调试的堆栈
	addl	%ebx, %esp

	# Set up TR to make Intel VT happy
	ltr	%di

	# Clear registers to allow for future extensions to the
	# 32-bit boot protocol 清空通用寄存器
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	xorl	%ebx, %ebx
	xorl	%ebp, %ebp
	xorl	%edi, %edi

	# Set up LDTR to make Intel VT happy
	lldt	%cx

	jmpl	*%eax	# Jump to the 32-bit entrypoint 跳转到32位入口(跳转到(protected_mode_jump)第一个参数boot_params.hdr.code32_start地址)，此时仍处于保护模式
SYM_FUNC_END(.Lin_pm32)
