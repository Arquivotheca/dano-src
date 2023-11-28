TARGET_NAME:=dv.decoder

SRC_DIR:=src/addons/media/decoders/dv

SRCS := dvDecoder.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-Idvlib -Idvlib/common/inc

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
	libbedv.a
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, dvlib, $(SRC_DIR_IN))