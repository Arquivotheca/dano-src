SRC_DIR:=src/kit/settings

TARGET_NAME := libsettings.so

SRCS :=  settings.cpp

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/inc/app_p -I$(BUILDHOME)/src/drivers/history

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

