SRC_DIR:=src/kit/jpegread

TARGET_NAME := libjpegread.so

SRCS :=  \
	jpeg6b.cpp

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 $(F_NO_EXCEPTIONS) -Wall -W -Wno-multichar #-fno-omit-frame-pointer -DDEBUG=1 -fno-inline -gdwarf-2

MY_INCLUDES := -I$(BUILDHOME)/src/inc/jpegread_p -I$(BUILDHOME)/srcx/ijg/jpeg-6b

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libjpeg6b.a libroot.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk
