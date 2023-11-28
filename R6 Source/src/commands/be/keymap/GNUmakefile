# makefile
TARGET_NAME:= keymap
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:= $(TARGET_NAME).rsrc

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:=

MY_INCLUDES :=  \
			   -I$(BUILDHOME)/src/inc -I$(BUILDHOME)/headers/net \
			   -I$(BUILDHOME)/src/inc/app_server_p \
			   -I$(BUILDHOME)/src/inc/app_p

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

