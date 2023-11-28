SRC_DIR:=src/kit/device

TARGET_NAME := libdevice.so

SRCS :=  \
	SerialPort.cpp \
	DigitalPort.cpp \
	Joystick.cpp \
	A2D.cpp \
	D2A.cpp

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

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

