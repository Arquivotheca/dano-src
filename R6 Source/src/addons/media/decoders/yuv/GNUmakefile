SRC_DIR:=src/addons/media/decoders/mpeg1video
MY_OBJ_OFFSET := yuvvideo.decoder.f
SRCS := \
					luts.cpp \
#
TYPE := SHARED
MY_INCLUDES := -I$(BUILDHOME)/src/addons/media/extractors/avi \
			-I$(BUILDHOME)/src/inc/media_p \
			-I$(call FULL_SRC_DIR, src/addons/media/decoders/mpeg1video) \
			-I$(call FULL_SRC_DIR, src/addons/media/decoders/yuv) \
#

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

# YUV Video Decoder
TARGET_NAME:=yuvvideo.decoder

SRC_DIR:=src/addons/media/decoders/yuv

SRCS := YUVDecoder.cpp convert.cpp

MY_INCLUDES := $(MY_INCLUDES_IN)

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

include $(SDIR_TEMPLATES)/AddonTemplate.mk

