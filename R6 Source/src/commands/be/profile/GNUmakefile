# makefile
TARGET_NAME:= profile
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS		:= profile.cpp

MY_CFLAGS:= -fexceptions

MY_INCLUDES	:=  -I$(BUILDHOME)/src/inc/os_p

SYSTEM_LIBS:= libroot.so libbe.so libdevel.a

ifneq ($(call MFLAG_IS_SET, SUPPORTS_COMPRESSED_ELF_LOADER),)
SYSTEM_LIBS += libuncrush.a libz.a
endif

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk

