# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.
#
# REMEMBER: You can't comment out any of these, though some of them can be
#  set to empty. (The flat makefile system means if you don't set things to
#  empty, they likely won't be empty when you're called.)

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libfraunhofermp3.a

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/addons/media/decoders/mpegaudio_fh/fraunhofer

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
          -DIGNORE_CRC \
          -DUSE_MP3DECIFC \
          -DSYNC_ALL_LAYERS \
          -DL2_FH_POLYPHASE

ifneq ($(call MFLAG_IS_SET, FIXED_POINT_DECODE),)
	MY_CFLAGS += -DENABLE_FIXED_POINT
else
	MY_CFLAGS += -DENABLE_FLOATING_POINT
endif

MY_CFLAGS += $(call MFLAG_TO_DFLAG, USE_3DNOW_INSTRUCTIONS, _USE_3DNOW)

ifeq ($(TARGET_CPU), i586)
	MP3ASM := corelibs/mp3dec/polyphase_asm.asm
	MY_CFLAGS += -D_USE_X86_ASM=1
else
	MP3ASM :=
endif

ifneq ($(call MFLAG_IS_SET, FIXED_POINT_DECODE),)
	MY_CFLAGS += $(call MFLAG_TO_DFLAG, USE_MMX_INSTRUCTIONS, _USE_MMX)
	MMXOBJS	:= corelibs/mp3dec/getbits_asm.asm
else
	MMXOBJS :=
endif

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)

MP3COREOBJS := $(MP3ASM) $(addprefix corelibs/mp3dec/, \
				bitstream.cpp conceal.cpp crc16.cpp huffdec.cpp huffmanbitobj.cpp \
                huffmandecoder.cpp huffmantable.cpp l3table.cpp mp3decinfo.cpp \
                mp3read.cpp mp3ssc.cpp \
                mpegbitstream.cpp mpegheader.cpp mpgadecoder.cpp \
                mp3decifc.cpp \
                mp2.cpp l2table.cpp )

FLOATOBJS       :=  $(addprefix corelibs/mp3dec/, \
				mp3decode.cpp mp2decode.cpp mp1decode.cpp mdct.cpp \
				polyphase.cpp mp3quant.cpp mp3tools.cpp )

FIXEDOBJS       := $(addprefix corelibs/mp3dec/, \
				mp3decode_int.cpp mp2decode_int.cpp mp1decode_int.cpp \
				mdct_int.cpp polyphase_int.cpp \
                mp3quant_int.cpp mp3tools_int.cpp \
                polyphase_int_asm.asm mdct_int_asm.asm \
                antialias_int_asm.asm mx31x16.cpp )

COMMONOBJS      := $(addprefix common/util/, \
				meanvalue.cpp sequencedetector.cpp )

BUFFERPARSEOBJS = tools/synctools/mp3bufferparse/mp3bufferparse.cpp

SRCS:= \
            $(MP3COREOBJS) \
            $(COMMONOBJS) \
            $(BUFFERPARSEOBJS) \
            $(MMXOBJS)

ifneq ($(call MFLAG_IS_SET, FIXED_POINT_DECODE),)
SRCS += $(FIXEDOBJS)
else
SRCS += $(FLOATOBJS)
endif

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR) \
			-I$(BUILDHOME)/$(SRC_DIR)/corelibs/mp3dec \
            -I$(BUILDHOME)/$(SRC_DIR)/common/include \
            -I$(BUILDHOME)/$(SRC_DIR)/common/util \
            -I$(BUILDHOME)/$(SRC_DIR)/tools/synctools/mp3bufferparse


# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
LINKS :=


MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

