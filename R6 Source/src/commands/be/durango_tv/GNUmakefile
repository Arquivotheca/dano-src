# app makefile
TARGET_NAME:= durango_tv
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).c

MY_CFLAGS:=

MY_INCLUDES	:= \
		-I$(BUILDHOME)/src/inc/os_p

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

