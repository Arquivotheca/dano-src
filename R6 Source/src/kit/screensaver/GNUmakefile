SRC_DIR:=src/kit/screensaver

TARGET_NAME := libscreensaver.so

SRCS := \
		ScreenSaver.cpp \
#

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
			-I$(BUILDHOME)/headers/add-ons/screen_saver \
#

MY_INSTALL_DIR := $(INSTALL_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := libroot.so libbe.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

