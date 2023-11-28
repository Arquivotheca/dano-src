
TARGET_NAME:= mail_parser
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_CFLAGS:= -DDEBUG=1

MY_FIRST_INCLUDES := -I$(BUILDHOME)/src/inc/os_p \
					 -I$(BUILDHOME)/src/drivers/scsiprobe

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
