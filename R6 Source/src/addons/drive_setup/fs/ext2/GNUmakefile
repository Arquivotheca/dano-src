
TARGET_NAME:=ext2

SRC_DIR:=src/addons/drive_setup/fs/$(TARGET_NAME)

SRCS := \
	$(TARGET_NAME).cpp


MY_CFLAGS := 

MY_INCLUDES:= \
		-I$(BUILDHOME)/$(SRC_DIR) \
		-I$(BUILDHOME)/src/inc/os_p \
#

SYSTEM_LIBS:=libroot.so libbe.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/drive_setup/fs

include $(SDIR_TEMPLATES)/AddonTemplate.mk

