
TARGET_NAME:= listdev
SRC_DIR := src/commands/be/$(TARGET_NAME)

# RESOURCES := $(TARGET_NAME).rsrc

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := listdev.c cm_wrapper.c

MY_CFLAGS:= 

MY_INCLUDES += -I$(BUILDHOME)/src/inc/driver

SYSTEM_LIBS:= libbe.so libroot.so libtextencoding.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
