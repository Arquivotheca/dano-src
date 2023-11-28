TARGET_NAME:=msvideo.decoder

SRC_DIR:=src/addons/media/decoders/msvideo

SRCS := MSVideo.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/addons/media/extractors/avi/

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

