SRC_DIR:=src/kit/tcpmessage

TARGET_NAME := libtcpmessage.so

SRCS :=  TCPMessenger.cpp TCPLooper.cpp

MY_CFLAGS := -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := -I$(BUILDHOME)/src/inc/udpmessage_p

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libsocket.so libbind.so libbe.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk
