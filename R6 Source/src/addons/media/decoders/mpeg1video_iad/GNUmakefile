TARGET_NAME:=mpeg1video.decoder

SRC_DIR:=src/addons/media/decoders/mpeg1video_iad

SRCS	:= MPEGVideoDecoder.cpp getbits.cpp getblk.cpp gethdr.cpp getpic.cpp getvlc.cpp \
			idct.cpp luts.cpp mpegdec.cpp recon.cpp \
			MPEGyuv2rgb.cpp \
			ycbcr_mmx.asm \
			idct_mmx.asm block_mmx.asm \
			mc_mmx8.asm mc_mmx16.asm mc2_mmx8.asm mc2_mmx16.asm 
		

MY_CFLAGS := 

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
#

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/decoders

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=

MY_OPTIMIZER:=
include $(SDIR_TEMPLATES)/AddonTemplate.mk
