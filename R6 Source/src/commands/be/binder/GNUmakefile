# app makefile
TARGET_NAME:= binder
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:=

MY_INCLUDES	:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
