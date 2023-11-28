TARGET_NAME:=pjpeg.decoder

SRC_DIR:=src/addons/media/decoders/pjpeg

SRCS := PJPEGcodec.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/addons/media/jpeglib \
	-I$(BUILDHOME)/src/addons/media/jpeglib/jpeglib/jpeg-6a

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
	libpjpeg-jpeg.a \
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

