TARGET_NAME:=ccittadpcm.decoder

SRC_DIR:=src/addons/media/decoders/ccittadpcm

SRCS := CCITTDecoder.cpp \
		g711.cpp \
		g721.cpp \
		g723_24.cpp \
		g723_40.cpp \
		g72x.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk


