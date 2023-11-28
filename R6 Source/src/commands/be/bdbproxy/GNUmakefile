# Deskbar makefile
TARGET_NAME:= bdbproxy
SRC_DIR := src/commands/be/$(TARGET_NAME)

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:= -DDEBUG=1

MY_INCLUDES	:= \
			   -I$(BUILDHOME)/src/inc/bdb_p

SYSTEM_LIBS:= libbe.so libroot.so \
	libbind.so libsocket.so libtcpmessage.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
