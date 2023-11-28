TARGET_NAME:=indeo5.decoder

SRC_DIR:=src/addons/media/decoders/indeo5

SRCS := Indeo5.cpp  \
        beoshive.c

MY_CFLAGS :=

MY_INCLUDES:= -I. -Iinclude \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
	libindeo_dec.a libtranslation.so \
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:= decode.rt/resource.rsrc

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, decode.rt, $(SRC_DIR_IN))
