
TARGET_NAME:=printer_watcher

SRC_DIR:=src/addons/smoove/printer_watcher

SRCS := printer_watcher.cpp	\
		smoove.cpp

MY_CFLAGS:=

MY_INCLUDES	:= \
				-I$(BUILDHOME)/src/inc/print_p \
				-I$(BUILDHOME)/src/inc/print_server_p \
				-I$(BUILDHOME)/$(SRC_DIR)

SYSTEM_LIBS:= libbe.so libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/smoove

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
