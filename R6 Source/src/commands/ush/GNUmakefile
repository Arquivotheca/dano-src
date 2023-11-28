# ush makefile
ifneq ($(call MFLAG_IS_SET, USH_AS_BOOT_SHELL),)
TARGET_NAME:= sh
else
TARGET_NAME:= ush
endif

SRC_DIR := src/commands/ush

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS :=  ush.c commands.c readline.c smbios.c getmacaddress.c binder.cpp

MY_INCLUDES	:= \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/net_p

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

