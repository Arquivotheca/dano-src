
TARGET_NAME:=desklink

SRC_DIR:=src/commands/$(TARGET_NAME)

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
	libstdc++.r4.so

SRCS		:= $(ALL_DOT_CPP)

MY_INCLUDES += \
			-I$(BUILDHOME)/src/inc/support_p

RESOURCES := $(ALL_DOT_RSRC)

MY_INSTALL_DIR:=$(INSTALL_BIN)

include $(SDIR_TEMPLATES)/AppTemplate.mk

