#
# arch/x86/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Linus Torvalds
# Changed by many, many contributors over the years.
#

# Sanitizer runtimes are unavailable and cannot be linked for early boot code.
KASAN_SANITIZE			:= n
KCSAN_SANITIZE			:= n
OBJECT_FILES_NON_STANDARD	:= y

# Kernel does not boot with kcov instrumentation here.
# One of the problems observed was insertion of __sanitizer_cov_trace_pc()
# callback into middle of per-cpu data enabling code. Thus the callback observed
# inconsistent state and crashed. We are interested mostly in syscall coverage,
# so boot code is not interesting anyway.
KCOV_INSTRUMENT		:= n

# If you want to preset the SVGA mode, uncomment the next line and
# set SVGA_MODE to whatever number you want.
# Set it to -DSVGA_MODE=NORMAL_VGA if you just want the EGA/VGA mode.
# The number is the same as you would ordinarily press at bootup.

SVGA_MODE	:= -DSVGA_MODE=NORMAL_VGA

targets		:= vmlinux.bin setup.bin setup.elf bzImage
targets		+= fdimage fdimage144 fdimage288 image.iso hdimage
subdir-		:= compressed

setup-y		+= a20.o bioscall.o cmdline.o copy.o cpu.o cpuflags.o cpucheck.o
setup-y		+= early_serial_console.o edd.o header.o main.o memory.o
setup-y		+= pm.o pmjump.o printf.o regs.o string.o tty.o video.o
setup-y		+= video-mode.o version.o
setup-$(CONFIG_X86_APM_BOOT) += apm.o

# The link order of the video-*.o modules can matter.  In particular,
# video-vga.o *must* be listed first, followed by video-vesa.o.
# Hardware-specific drivers should follow in the order they should be
# probed, and video-bios.o should typically be last.
setup-y		+= video-vga.o
setup-y		+= video-vesa.o
setup-y		+= video-bios.o

targets		+= $(setup-y)
hostprogs	:= tools/build
hostprogs	+= mkcpustr

HOST_EXTRACFLAGS += -I$(srctree)/tools/include \
		    -include include/generated/autoconf.h \
	            -D__EXPORTED_HEADERS__

ifdef CONFIG_X86_FEATURE_NAMES
$(obj)/cpu.o: $(obj)/cpustr.h

quiet_cmd_cpustr = CPUSTR  $@
      cmd_cpustr = $(obj)/mkcpustr > $@
$(obj)/cpustr.h: $(obj)/mkcpustr FORCE
	$(call if_changed,cpustr)
endif
targets += cpustr.h

# ---------------------------------------------------------------------------

KBUILD_CFLAGS	:= $(REALMODE_CFLAGS) -D_SETUP
KBUILD_AFLAGS	:= $(KBUILD_CFLAGS) -D__ASSEMBLY__
KBUILD_CFLAGS	+= $(call cc-option,-fmacro-prefix-map=$(srctree)/=)
KBUILD_CFLAGS	+= -fno-asynchronous-unwind-tables
GCOV_PROFILE := n
UBSAN_SANITIZE := n

$(obj)/bzImage: asflags-y  := $(SVGA_MODE)

quiet_cmd_image = BUILD   $@
silent_redirect_image = >/dev/null
cmd_image = $(obj)/tools/build $(obj)/setup.bin $(obj)/vmlinux.bin \
			       $(obj)/zoffset.h $@ $($(quiet)redirect_image)

# bzImage依赖于$(obj)/setup.bin $(obj)/vmlinux.bin $(obj)/tools/build, build为arch/x86/boot/tools/build工具, 生成bzImage
# build 查看arch/x86/boot/.bzImage.cmd
$(obj)/bzImage: $(obj)/setup.bin $(obj)/vmlinux.bin $(obj)/tools/build FORCE
	$(call if_changed,image)
	@$(kecho) 'Kernel: $@ is ready' ' (#'`cat .version`')'

# vmlinux.bin依赖于vmlinux, arch/x86/boot/compressed/Makefile
OBJCOPYFLAGS_vmlinux.bin := -O binary -R .note -R .comment -S
$(obj)/vmlinux.bin: $(obj)/compressed/vmlinux FORCE
	$(call if_changed,objcopy)

SETUP_OBJS = $(addprefix $(obj)/,$(setup-y))

sed-zoffset := -e 's/^\([0-9a-fA-F]*\) [a-zA-Z] \(startup_32\|startup_64\|efi32_stub_entry\|efi64_stub_entry\|efi_pe_entry\|efi32_pe_entry\|input_data\|kernel_info\|_end\|_ehead\|_text\|z_.*\)$$/\#define ZO_\2 0x\1/p'

quiet_cmd_zoffset = ZOFFSET $@
      cmd_zoffset = $(NM) $< | sed -n $(sed-zoffset) > $@

targets += zoffset.h
$(obj)/zoffset.h: $(obj)/compressed/vmlinux FORCE
	$(call if_changed,zoffset)


AFLAGS_header.o += -I$(objtree)/$(obj)
$(obj)/header.o: $(obj)/zoffset.h

# setup.elf依赖于arch/x86/boot/setup.ld *.o，然后执行ld, arch/x86/boot/.setup.elf.cmd
LDFLAGS_setup.elf	:= -m elf_i386 -z noexecstack -T
$(obj)/setup.elf: $(src)/setup.ld $(SETUP_OBJS) FORCE
	$(call if_changed,ld)

# setup.bin依赖于setup.elf, 找到arch/x86/boot/.setup.elf.cmd
# cmd_arch/x86/boot/setup.elf := ld -m elf_x86_64   -m elf_i386 -T arch/x86/boot/setup.ld  \ 
# arch/x86/boot/a20.o arch/x86/boot/bioscall.o arch/x86/boot/cmdline.o arch/x86/boot/copy.o \ 
# arch/x86/boot/cpu.o arch/x86/boot/cpuflags.o arch/x86/boot/cpucheck.o arch/x86/boot/early_serial_console.o \ 
# arch/x86/boot/edd.o arch/x86/boot/header.o arch/x86/boot/main.o arch/x86/boot/memory.o arch/x86/boot/pm.o \ 
# arch/x86/boot/pmjump.o arch/x86/boot/printf.o arch/x86/boot/regs.o arch/x86/boot/string.o arch/x86/boot/tty.o \ 
# arch/x86/boot/video.o arch/x86/boot/video-mode.o arch/x86/boot/version.o arch/x86/boot/video-vga.o \ 
# arch/x86/boot/video-vesa.o arch/x86/boot/video-bios.o -o arch/x86/boot/setup.elf
OBJCOPYFLAGS_setup.bin	:= -O binary
$(obj)/setup.bin: $(obj)/setup.elf FORCE
	$(call if_changed,objcopy)

$(obj)/compressed/vmlinux: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

# Set this if you want to pass append arguments to the
# bzdisk/fdimage/hdimage/isoimage kernel
FDARGS =
# Set this if you want one or more initrds included in the image
FDINITRD =

imgdeps = $(obj)/bzImage $(obj)/mtools.conf $(src)/genimage.sh

$(obj)/mtools.conf: $(src)/mtools.conf.in
	sed -e 's|@OBJ@|$(obj)|g' < $< > $@

targets += mtools.conf

# genimage.sh requires bash, but it also has a bunch of other
# external dependencies.
quiet_cmd_genimage = GENIMAGE $3
cmd_genimage = $(BASH) $(srctree)/$(src)/genimage.sh $2 $3 $(obj)/bzImage \
		$(obj)/mtools.conf '$(FDARGS)' $(FDINITRD)

PHONY += bzdisk fdimage fdimage144 fdimage288 hdimage isoimage

# This requires write access to /dev/fd0
# All images require syslinux to be installed; hdimage also requires
# EDK2/OVMF if the kernel is compiled with the EFI stub.
bzdisk: $(imgdeps)
	$(call cmd,genimage,bzdisk,/dev/fd0)

fdimage fdimage144: $(imgdeps)
	$(call cmd,genimage,fdimage144,$(obj)/fdimage)
	@$(kecho) 'Kernel: $(obj)/fdimage is ready'

fdimage288: $(imgdeps)
	$(call cmd,genimage,fdimage288,$(obj)/fdimage)
	@$(kecho) 'Kernel: $(obj)/fdimage is ready'

hdimage: $(imgdeps)
	$(call cmd,genimage,hdimage,$(obj)/hdimage)
	@$(kecho) 'Kernel: $(obj)/hdimage is ready'

isoimage: $(imgdeps)
	$(call cmd,genimage,isoimage,$(obj)/image.iso)
	@$(kecho) 'Kernel: $(obj)/image.iso is ready'
