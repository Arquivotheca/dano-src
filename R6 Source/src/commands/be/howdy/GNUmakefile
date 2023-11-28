# app makefile
TARGET_NAME:= howdy
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:= 

MY_INCLUDES		:= -I$(BUILDHOME)/src/inc/kernel_p \
                $(INCLUDES) -I$(BUILDHOME)/src/inc \
                -I$(BUILDHOME)/src/inc/os_p -I$(BUILDHOME)/src/inc/app_p

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
