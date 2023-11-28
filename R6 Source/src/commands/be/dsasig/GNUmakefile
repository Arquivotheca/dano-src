# app makefile
TARGET_NAME:= dsasig
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).c

MY_CFLAGS:= 

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/app_p

SYSTEM_LIBS:= libbe.so libopenssl.so libroot.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk

# host app makefile

TARGET_NAME:= dsasig

SRC_DIR := src/commands/be/$(TARGET_NAME)

SRCS := $(TARGET_NAME).c

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/app_p  -I$(BUILDHOME)/src/commands/be/dsasig

MORE_OBJS := libopenssl.a

SYSTEM_LIBS:= libbe.so libroot.so

include $(SDIR_TEMPLATES)/HostAppTemplate.mk
