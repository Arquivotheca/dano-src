SRC_DIR:=src/kit/textencoding

TARGET_NAME := libtextencoding.so

SRCS :=  \
	EncodingConversions.cpp \
	ISOUnicodeMappings.cpp \
	MSUnicodeMappings.cpp \
	DOSUnicodeMappings.cpp \
	MACUnicodeMappings.cpp \
	ASIAUnicodeMappings.cpp
#
ifneq ($(call MFLAG_IS_SET,LIBTEXTENCODING_SUPPORTS_CHINESE),)

#
SRCS += \
	ChineseEncodings.cpp
endif

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 
MY_CFLAGS += $(call MFLAG_TO_DFLAG, LIBTEXTENCODING_SUPPORTS_CHINESE)

MY_INCLUDES := -I$(BUILDHOME)/headers/support \
				-I$(BUILDHOME)/$(SRC_DIR)

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

