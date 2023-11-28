# app makefile
TARGET_NAME:= bootmode
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).c mk_factory_settings.c

MY_CFLAGS:=

MY_INCLUDES	:= \
		-I$(BUILDHOME)/src/inc/driver -I$(BUILDHOME)/src/inc/os_p -I$(BUILDHOME)/src/zrecover/stage_2/include

SYSTEM_LIBS:= libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
