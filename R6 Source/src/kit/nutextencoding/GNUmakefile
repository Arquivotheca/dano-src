SRC_DIR:=src/kit/nutextencoding

TARGET_NAME := libnutextencoding.so

SRCS :=  \
	BTextCodec.cpp \
	BTextEncoding.cpp \
	BTextEncodingAddOn.cpp \
	TextEncodingNames.cpp \
	UTF8.cpp

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 

MY_INCLUDES := 
MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

