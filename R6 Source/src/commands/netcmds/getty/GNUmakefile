# app makefile
TARGET_NAME:= getty
SRC_DIR := src/commands/netcmds/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS 		:= getty.cpp

MY_CFLAGS:=

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/net_p \
			   -I$(BUILDHOME)/src/drivers/tty
			   
SYSTEM_LIBS:= libdevice.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

