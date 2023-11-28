TARGET_NAME:=cinepak.media2_addon

SRC_DIR:=src/addons/media2/decoders/cinepak

SRCS := Cinepak.cpp		\
		CinepakAddon.cpp	\
#

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so \
	libcinepak.a
#

HAS_MAIN:= false

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, libsrc, $(SRC_DIR_IN))