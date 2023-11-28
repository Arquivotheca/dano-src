TARGET_NAME:=ccittadpcm.media2_addon

SRC_DIR:=src/addons/media2/decoders/ccittadpcm

SRCS := CCITTDecoder.cpp \
		CCITTDecoderAddon.cpp \
		g711.cpp \
		g721.cpp \
		g723_24.cpp \
		g723_40.cpp \
		g72x.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media2_p 

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk


