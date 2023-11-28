TARGET_NAME:=mpeg1audio_fh.media2_addon

SRC_DIR:=src/addons/media2/decoders/mpegaudio_fh

SRCS := MPEGAudioDecoder.cpp	\
		MPEGAudioAddon.cpp		\
#

MY_CFLAGS :=

MY_CFLAGS += $(call MFLAG_TO_DFLAG, FIXED_POINT_DECODE)

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/addons/media2/decoders/mpegaudio_fh/fraunhofer/corelibs/mp3dec \
	-I$(BUILDHOME)/src/inc/media2_p

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so libfraunhofermp3.a

PARENT_IMAGE:=

HAS_MAIN:= false

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(BUILDHOME)/src/addons/media2/decoders/mpegaudio_fh/fraunhofer/$(MAKEFILE_LEAFNAME)

