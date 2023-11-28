SRC_DIR:=src/kit/prefpanel

TARGET_NAME := libprefpanel.so

SRCS :=  \
	Helpers.cpp \
	ListAddOns.cpp \
	PrefsApp.cpp \
	PPAddOn.cpp

MY_INCLUDES := -I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/device_p \
			-I$(BUILDHOME)/src/inc/driver \
			-I$(BUILDHOME)/headers/device

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

