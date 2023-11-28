# makefile
TARGET_NAME:= mountvolume
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:= 

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := main.cpp

MY_CFLAGS:= 

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc \

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

