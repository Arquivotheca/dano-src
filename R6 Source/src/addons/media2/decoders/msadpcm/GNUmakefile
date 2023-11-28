TARGET_NAME:=msadpcm.media2_addon

SRC_DIR:=src/addons/media2/decoders/msadpcm

SRCS	:= MSADPCMDecoder.cpp \
			MSADPCMDecoderAddon.cpp \
			msadpcm_decode.cpp	\
#

MY_CFLAGS :=

MY_INCLUDES:= \
				-I$(BUILDHOME)/src/inc/media2_p \
				-I$(BUILDHOME)/src/addons/media/extractors/avi
#

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
