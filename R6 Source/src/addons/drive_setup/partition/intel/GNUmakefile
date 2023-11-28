
TARGET_NAME:=intel

SRC_DIR:=src/addons/drive_setup/partition/$(TARGET_NAME)

SRCS := intel.cpp intel_map.c intel_types.c

MY_CFLAGS := 

MY_INCLUDES:= \
		-I$(BUILDHOME)/$(SRC_DIR) \
		-I$(BUILDHOME)/src/inc/os_p \
		-I$(BUILDHOME)/src/inc/interface_p \
#

SYSTEM_LIBS:=libroot.so libbe.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/drive_setup/partition

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

