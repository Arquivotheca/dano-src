TARGET_NAME:=cinepak.decoder

SRC_DIR:=src/addons/media/decoders/cinepak

SRCS := Cinepak.cpp 

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so \
	libcinepak.a
#

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, libsrc, $(SRC_DIR_IN))