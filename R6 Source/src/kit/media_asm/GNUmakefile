SRC_DIR:=src/kit/media_asm

TARGET_NAME := libmedia_asm.a

SRCS := \
	mixer_i586.S \
	converter_i586.S

MY_CFLAGS := -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/media_p

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

SYSTEM_LIBS :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

