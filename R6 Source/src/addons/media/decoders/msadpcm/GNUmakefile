TARGET_NAME:=msadpcm.decoder

SRC_DIR:=src/addons/media/decoders/msadpcm

SRCS	:= MSADPCMDecoder.cpp \
		msadpcm_decode.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/addons/media/extractors/avi

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
