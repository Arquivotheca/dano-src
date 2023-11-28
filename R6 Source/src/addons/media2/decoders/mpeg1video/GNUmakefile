TARGET_NAME:=mpeg1video.decoder

SRC_DIR:=src/addons/media2/decoders/mpeg1video

SRCS := MPEGVideoDecoder.cpp \
		luts.cpp \
		getbits.cpp \
		getblk.cpp \
		gethdr.cpp \
		getpic.cpp \
		getvlc.cpp \
		idct.cpp \
		mpegdec.cpp \
		recon.cpp \
		MPEGyuv2rgb.cpp

MY_CFLAGS +=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media2_p \
	-Immx
#

SYSTEM_LIBS:= libroot.so libbe2.so libmedia2.so mpegmmx.a

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media2/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, mmx, $(SRC_DIR_IN))
