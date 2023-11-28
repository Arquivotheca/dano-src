TARGET_NAME:=rgbvideo.decoder

SRC_DIR:=src/addons/media/decoders/rgb

SRCS := RGBCodec.cpp convert.cpp

MY_CFLAGS :=

MY_INCLUDES := -I$(BUILDHOME)/src/addons/media/extractors/avi \
            -I$(BUILDHOME)/src/inc/media_p

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

