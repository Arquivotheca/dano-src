# Makefile
TARGET_NAME:=gameaudioplay
SRC_DIR:=src/commands/$(TARGET_NAME)

SRCS:=$(TARGET_NAME).cpp

MY_CFLAGS:=
MY_INCLUDES:=-I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so
RESOURCE_FORK:=
LINK_DIRS:=
MY_INSTALL_DIR:=$(INSTALL_BIN)


include $(SDIR_TEMPLATES)/AppTemplate.mk
