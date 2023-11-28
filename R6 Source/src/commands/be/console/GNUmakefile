# app makefile
TARGET_NAME:= console
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := console.c \
		ptystuff.c \
		vt100.c

MY_CFLAGS:=

MY_INCLUDES	:= \
	-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS:= libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
