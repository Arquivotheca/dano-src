SRC_DIR:=src/kit/uncrush

TARGET_NAME := libuncrush.a

SRCS :=         \
	uncrush.c \
	depak.c \
	misc_depak.c

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := 	-I$(BUILDHOME)/headers/devel \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc \
				-I$(BUILDHOME)/src/tools/crushers/elf_to_celf

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

SYSTEM_LIBS :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)

TARGET_NAME := $(TARGET_NAME_IN)

SRCS := $(SRCS_IN)

MY_CFLAGS :=  $(MY_CFLAGS_IN)

MY_INCLUDES := $(MY_INCLUDES_IN)

SYSTEM_LIBS :=

LINKS:=

include $(SDIR_TEMPLATES)/HostStaticLibTemplate.mk


