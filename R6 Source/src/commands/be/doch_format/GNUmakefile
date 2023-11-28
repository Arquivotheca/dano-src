# makefile
TARGET_NAME:= doch_format
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := dformat.cpp

MY_CFLAGS:= 

MY_INCLUDES := -I$(BUILDHOME)/src/drivers/disk_on_chip/src \
			-I$(BUILDHOME)/src/drivers/disk_on_chip/custom \
			-I$(BUILDHOME)/src/drivers/disk_on_chip/systems/beos \
			-I$(BUILDHOME)/src/nukernel/inc 

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

