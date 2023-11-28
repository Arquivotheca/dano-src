# testmike
TARGET_NAME:=testmike
SYSTEM_LIBS:= libroot.so libbe.so libmedia.so
MY_INCLUDES:= -I$(BUILDHOME)/src/inc/driver
SRC_DIR:=src/commands/$(TARGET_NAME)
SRCS:=$(ALL_DOT_CPP)
RESOURCE_FORK:=testmike.rsrc
LINK_DIRS:= $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
MY_INSTALL_DIR:=$(INSTALL_VALIDATE)
MY_CFLAGS:=
include $(SDIR_TEMPLATES)/AppTemplate.mk
