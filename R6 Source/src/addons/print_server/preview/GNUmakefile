# makefile

SRC_DIR := src/addons/print_server/preview

TARGET_NAME := Preview

SRCS :=  	\
		PreviewDriver.cpp	\
		PreviewWindow.cpp

MY_CFLAGS := -DUSE_OFFS=0
MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR)/add_ons -I$(BUILDHOME)/$(SRC_DIR)

SYSTEM_LIBS := libroot.so libbe.so
PARENT_IMAGE :=

HAS_MAIN :=

MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Print

LINK_DIRS :=
SIGNATURE :=
RESOURCES :=
ATTRIBUTES :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
