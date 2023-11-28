TARGET_NAME:=	indeo5.media2_addon

SRC_DIR:= src/addons/media2/decoders/indeo5

SRCS := \
			Indeo5.cpp \
			beoshive.c \
			Indeo5Addon.cpp \
#

MY_DFLAGS :=

MY_DFLAGS +=

MY_INCLUDES:= include
#

SYSTEM_LIBS:= libroot.so libbe.so libbe2.so libmedia2.so libindeo_dec.a libtranslation.so
#

HAS_MAIN:= false

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

RESOURCES:= decode.rt/resource.rsrc

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, decode.rt, $(SRC_DIR_IN))
