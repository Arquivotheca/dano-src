# zbeos makefile

# These are used by all the subcomponents of 'beos'.
MY_CFLAGS:= -fno-pic -Os -D_STATIC_LINKING=1 -DINTEL \
			-DI586=1 -Di586=1 -DI486=1 -D_BE=1 -DFIXED=1 \
			-DBOOT -DBOOTSTRAP_LOADER_EXISTS \
			-DSINGLE_THREADED_ERRNO=1 \
			-fomit-frame-pointer -malign-loops=0 -malign-jumps=0 \
			-malign-functions=0

# At this point only -I. and -I.. (not -I../..) are supported
MY_INCLUDES:= \
		-I$(BUILDHOME)/src/addons/drive_setup/partition/intel \
		-I$(BUILDHOME)/src/boot/gr \
		-I$(BUILDHOME)/src/fs/bfs \
		-I$(BUILDHOME)/src/fs/cfs \
		-I$(BUILDHOME)/src/inc/driver \
		-I$(BUILDHOME)/src/inc/os_p \
		-I$(BUILDHOME)/src/nukernel/inc \
		-I$(BUILDHOME)/src/nukernel/lib/inc \
		-I$(BUILDHOME)/src/nukernel/arch/isa \
		-I$(BUILDHOME)/src/nukernel/arch/intel \
    	-I$(BUILDHOME)/src/nukernel/vm/$(call MVAR_TO_TARGET_NAME, SYSTEM_KERNEL_FEATURE_VM_FLAVOUR)/common \
    	-I$(BUILDHOME)/src/nukernel/vm/$(call MVAR_TO_TARGET_NAME, SYSTEM_KERNEL_FEATURE_VM_FLAVOUR)/cpu/$(TARGET_CPU) \
		-I$(BUILDHOME)/src/nukernel/cpu/i586 \
		-I$(call FULL_SRC_DIR, src/kit/malloc) \
		-I$(BUILDHOME)/srcx/gnu/zlib-1.1.3 \
#

MY_CFLAGS += $(call MFLAG_TO_DFLAG, SUPPORTS_RELEASE_MODE, _SUPPORTS_RELEASE_MODE)

# MORE_OBJS gathers the objects for 'beos'
MORE_OBJS:=

# the stuff from the kernel
TYPE:= BOOTLOADER
SRC_DIR:= src/nukernel
MY_OBJ_OFFSET := boot_$(TARGET_PLATFORM)
SRCS := \
		lib/src/alloc.c \
		cpu/$(TARGET_CPU)/bootdata.c \
		lib/src/driver_settings.c \
		lib/src/printf.c \
		lib/src/sio.c \
		arch/isa/sio16550.c
include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS += $(OBJS_OUT)

# the stuff from srcx/c-extra 
SRC_DIR := srcx/c-extra
SRCS := \
	memcpy.S \
	memset.S
MY_OBJ_OFFSET := boot_$(TARGET_PLATFORM)
TYPE:= BOOTLOADER
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS += $(OBJS_OUT)

# The stuff from the kernel kit
SRC_DIR := src/kit/kernel
SRCS := \
		i586/byteorder_x86.S
MY_OBJ_OFFSET := boot_$(TARGET_PLATFORM)
TYPE:= BOOTLOADER
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS += $(OBJS_OUT)

# The stuff from the malloc kit
SRC_DIR := src/kit/malloc
SRCS := malloc.c
MY_OBJ_OFFSET := boot_$(TARGET_PLATFORM)
TYPE:= BOOTLOADER
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS += $(OBJS_OUT)

SRC_DIR:=src/boot/x86

TARGET_NAME:= beos

TYPE:= BOOTLOADER

# This must be first
SRCS := \
		crt0.S

# Intel-specific code
SRCS += \
		bios.c \
		biosid.c \
		disk.c \
		main.c \
		platform.c \
		vesa.c \
		video.c

# "Platform-independent code"
SRCS += \
		bootmenu.c \
		bt_bfs.c \
		bt_cfs.c \
		bt_cpu_asm.S \
		bt_devfs.c \
		bt_ffs.c \
		bt_filelist.c \
		bt_fs.c \
		bt_misc.c \
		cfs/cfs_compress.c \
		fmap.c \
		inflate.c \
		intel_partition/intel_map.c \
		load_elf.c

ifneq ($(call MFLAG_IS_SET,BOOT_PERSONAL_EDITION_IMAGES),)
SRCS += \
		bt_dos.c \
		bt_ext2.c \
		bt_ntfs.c \
#
endif

MY_INCLUDES := $(MY_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)

COBJ_DIR := $(call FULL_OBJ_DIR, src/kit/glibc)

# GLIBC objects
MORE_OBJS += \
		$(COBJ_DIR)/string/memmove.o \
		$(COBJ_DIR)/string/strncmp.o \
		$(COBJ_DIR)/string/strncpy.o \
		$(COBJ_DIR)/string/strcmp.o \
		$(COBJ_DIR)/string/strcpy.o \
		$(COBJ_DIR)/string/strchr.o \
		$(COBJ_DIR)/string/strrchr.o \
		$(COBJ_DIR)/string/strstr.o \
		$(COBJ_DIR)/stdlib/strtoul.o \
		$(COBJ_DIR)/ctype/ctype-info.o \
		$(COBJ_DIR)/wcsmbs-utf/btowc.o \
		$(COBJ_DIR)/locale/C-ctype.o \
		$(COBJ_DIR)/locale/lc-numeric.o \
		$(COBJ_DIR)/locale/C-numeric.o \
		$(COBJ_DIR)/locale/C_name.o \
	$(LIBGCC)

SYSTEM_LIBS:=

ifneq ($(call MFLAG_IS_SET, SUPPORTS_COMPRESSED_ELF_LOADER),)
SYSTEM_LIBS+= libuncrush.a libz.a
endif

include $(SDIR_TEMPLATES)/ComplexTemplate.mk

#
# ComplexTemplate needs to know how to put 'z' together, as the link rule is the
#   same as the bootloader. (The bootloader needs the multi-dir stuff.)
#

SRC_DIR:=src/boot/x86
TARGET_NAME:= z
TYPE:= BOOTLOADER

MY_OBJ_OFFSET := $(TARGET_NAME).f

SRCS:= z.S inflate.c

# Inherit these from 'beos'
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)

include $(SDIR_TEMPLATES)/ComplexTemplate.mk

#
# The rest of the makefile items are handled by the GenericGeneratedTemplate
#
SRC_DIR:=src/boot/x86
OBJ_DIR:=$(OBJ_DIR_FROM_SRC_DIR)

SRC_DIR:=src/boot/x86
TARGET_NAME := beos.gz
MY_DEPS := $(OBJ_DIR)/beos
MY_INSTALL_DIR :=
MY_RULE := GZIP_RULE
include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

SRC_DIR:=src/boot/x86
TARGET_NAME := shell
MY_DEPS := $(BUILDHOME)/$(SRC_DIR)/shell.S
MY_INSTALL_DIR :=
MY_RULE := ASSEMBLE16_RULE
include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

#
# Now we put together the above mentioned treats
#
SRC_DIR:=src/boot/x86
OBJ_DIR:=$(OBJ_DIR_FROM_SRC_DIR)

TARGET_NAME := zbeos
MY_DEPS := $(OBJ_DIR)/shell $(OBJ_DIR)/z $(OBJ_DIR)/beos.gz

ifneq ($(call MFLAG_IS_SET, SUPPORTS_BOOTLOADER_WITH_IMAGES),)
	MY_DEPS += $(BUILDHOME)/$(SRC_DIR)/images.gz
endif

MY_INSTALL_DIR := $(INSTALL_SYSTEM)
MY_RULE := CONCAT_RULE
include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

