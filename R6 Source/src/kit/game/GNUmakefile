SRC_DIR:=src/kit/game

TARGET_NAME := libgame.so

SRCS := \
		WindowScreen.cpp \
		DirectWindow.cpp \
		DirectDriver.cpp \
		DirectDriver1.cpp \
		FileGameSound.cpp \
		GameSound.cpp \
		PrivGameSound.cpp \
		PushGameSound.cpp \
		SimpleGameSound.cpp \
		StreamingGameSound.cpp


MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
			-I$(BUILDHOME)/$(SRC_DIR) \
			-I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/app_server_p \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/interface_p \
			-I$(BUILDHOME)/src/inc/input_server_p \
			-I$(BUILDHOME)/src/inc/media_p \
			-I$(BUILDHOME)/src/inc \
			-I$(BUILDHOME)/headers/add-ons/graphics \
			-I$(BUILDHOME)/headers/media

MY_INSTALL_DIR := $(INSTALL_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := libroot.so libbe.so libmedia.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

