
TARGET_NAME:= clipboard
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCES :=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:= -DDEBUG=1

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/app_p

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
