# allan 8-13-00
# this makefile now only does read-only translators 
# steven 8-24-00
# read-write enabled.
# jbq 1-29-01
# JPEG makefile created from the PNG one

TARGET_NAME:=JPEGTranslator

SRC_DIR:=src/addons/translators2/JPEG

SRCS	:= JPEGTranslator.cpp Decode.cpp 

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/srcx/gnu/jpeg-6a

SYSTEM_LIBS:= libroot.so libbe.so libtranslation.so libjpeg.a

PARENT_IMAGE:=

MY_CFLAGS += $(call MFLAG_TO_DFLAG, SUPPORTS_FEATURE_TRANSLATORS_CAN_WRITE, _SUPPORTS_READ_WRITE_TRANSLATORS)

HAS_MAIN:=0
ifneq ($(call MFLAG_IS_SET, SUPPORTS_FEATURE_TRANSLATORS_CAN_WRITE),)
SRCS			+= Encode.cpp Prefs.cpp
HAS_MAIN:=1
endif

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/Translators

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
