TARGET_NAME:=	ac3audio.media2_addon

SRC_DIR:= src/addons/media2/decoders/ac3audio

SRCS := \
			AC3AudioAddon.cpp \
			AC3Decoder.cpp \
#

MY_DFLAGS :=

MY_DFLAGS +=

MY_INCLUDES:= -Ilibac3audio -I../mpeg2video/libmpeg2video
#

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so libac3audio.a libMPEG2.so
#

HAS_MAIN:= false

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

include $(SDIR_TEMPLATES)/AddonTemplate.mk

TEMP_SRC_DIR_IN := $(SRC_DIR_IN)
include $(call INCLUDE_IF_AVAILABLE, libac3audio, $(SRC_DIR_IN))

