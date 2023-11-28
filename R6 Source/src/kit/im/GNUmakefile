SRC_DIR:=src/kit/im

TARGET_NAME := libim.so

SRCS := IMManager.cpp \
		IMMessageContainer.cpp \
		IMTransport.cpp \
		IMTransportHandle.cpp \
		IMChannelManager.cpp \
		IMChannel.cpp

MY_CFLAGS := -DDO_AUTHENTICATION=1 -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
		-I$(BUILDHOME)/headers/im \
		-I$(BUILDHOME)/headers/www
		
MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so libsocket.so libbind.so libwww.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

