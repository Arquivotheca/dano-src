SRC_DIR:=src/kit/bitflinger

TARGET_NAME := libbitflinger.so

SRCS := Bitflinger.cpp \
		asm.c \
		Cache.cpp \
		Converter.cpp \
		Default.cpp \
		Extractor.cpp \
		Float_x86_Core.cpp \
		Int_x86_Core.cpp \
		MMX_x86_Core.cpp

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := 

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so 

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

