TARGET_NAME:=	mpeg2video.media2_addon

SRC_DIR:= src/addons/media2/decoders/mpeg2video

SRCS := \
			MPEG2VideoAddon.cpp \
			MPEG2VideoDecoder.cpp \
#

MY_DFLAGS :=

MY_DFLAGS +=

MY_INCLUDES:= -Ilibmpeg2video
#

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so libmpeg2video.a libMPEG2.so
#

HAS_MAIN:= false

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

include $(SDIR_TEMPLATES)/AddonTemplate.mk

TEMP_SRC_DIR_IN := $(SRC_DIR_IN)
include $(call INCLUDE_IF_AVAILABLE, libmpeg2video, $(SRC_DIR_IN))
include $(call INCLUDE_IF_AVAILABLE, mpeglib-multiplatform, $(TEMP_SRC_DIR_IN))

