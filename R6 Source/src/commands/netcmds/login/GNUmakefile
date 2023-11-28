# app makefile
TARGET_NAME:= login
SRC_DIR := src/commands/netcmds/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS 		:= login.c

MY_CFLAGS:=

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/net_p \
			   -I$(BUILDHOME)/src/drivers/tty
			   
SYSTEM_LIBS:= libroot.so 

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk

