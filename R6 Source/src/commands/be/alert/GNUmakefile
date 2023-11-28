
TARGET_NAME:= alert
SRC_DIR := src/commands/be/$(TARGET_NAME)

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:= -DDEBUG=1

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/app_p

SYSTEM_LIBS:= libbe.so libroot.so

include $(SDIR_TEMPLATES)/AppTemplate.mk
