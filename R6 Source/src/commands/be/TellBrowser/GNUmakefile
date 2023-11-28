# Deskbar makefile
TARGET_NAME:= TellBrowser
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:= $(TARGET_NAME).rsrc

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := BrowserControl.cpp wagner/BrowserWindow.cpp wagner/ViewTree.cpp \
	CrashWindow.cpp

MY_CFLAGS:=

MY_INCLUDES	:= \
	-I$(BUILDHOME)/src/apps/wagner -I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/drivers/history -I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/src/kit/interface -I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/os_p

ifneq ($(call MFLAG_IS_SET, SUPPORT_JAPANESE),)
	MY_CFLAGS += -DSUPPORT_JAPANESE=1
	SRCS +=  JapaneseControl.cpp
	MY_INCLUDES += -I$(BUILDHOME)/src/addons/input_server/methods/japanese
endif

SYSTEM_LIBS:= libbe.so libwww.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
