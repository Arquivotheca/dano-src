TARGET_NAME:=powermgmt

SRC_DIR:=src/addons/input_server/filters/powermgmt

SRCS	:= PowerMgmtController.cpp PowerMgmtFilter.cpp

MY_CFLAGS := 

MY_INCLUDES:= \
	-I$(BUILDHOME)/headers/add-ons/input_server \
	-I$(BUILDHOME)/src/inc

SYSTEM_LIBS:= libroot.so libbe.so

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/filters

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
