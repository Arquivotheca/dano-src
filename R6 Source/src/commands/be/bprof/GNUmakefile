#  makefile
TARGET_NAME:= bprof
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).c

MY_CFLAGS:=

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/kernel_p \
               -I$(BUILDHOME)/src/inc \
               -I$(BUILDHOME)/src/inc/os_p \
               -I$(BUILDHOME)/headers/devel

SYSTEM_LIBS:= libdevel.a libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
