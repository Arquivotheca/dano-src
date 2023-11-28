SRC_DIR:=src/kit/font2

TARGET_NAME := libfont2.so

FFDIR		= fontfusion/core

SRCS :=	\
	$(FFDIR)/autogrid.c \
	$(FFDIR)/fft1hint.c \
	$(FFDIR)/fnt.c \
	$(FFDIR)/ghints.c \
	$(FFDIR)/glyph.c \
	$(FFDIR)/hstrat97.c \
	$(FFDIR)/scoder.c \
	$(FFDIR)/shapet.c \
	$(FFDIR)/sniffer.c \
	$(FFDIR)/strkconv.c \
	$(FFDIR)/t1.c \
	$(FFDIR)/t2k.c \
	$(FFDIR)/t2kextra.c \
	$(FFDIR)/t2ksbit.c \
	$(FFDIR)/t2ksc.c \
	$(FFDIR)/t2kstrk1.c \
	$(FFDIR)/t2kstrm.c \
	$(FFDIR)/t2ktt.c \
	$(FFDIR)/truetype.c \
	$(FFDIR)/tsimem.c \
	$(FFDIR)/util.c \
	FontEngine.cpp
#

#	These are unused files to support font types
#	we have never supported, and are unlikely to support
#	removing them saves some small amount of memory
#	$(FFDIR)/orion.c \
#	$(FFDIR)/pclread.c \
#	$(FFDIR)/pfrread.c \
#	$(FFDIR)/spdread.c \


MY_CFLAGS :=  \
	-D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 
#

# MVAR FF_CAPABILITY
#	configures the FontFusion engine into one of several well defined modes

# cflags to be set
#	ENABLE_ASSERT
#	ENABLE_LINE_LAYOUT
#	ENABLE_KERNING
#	LAYOUT_CACHE_SIZE somesize
#	ALGORITHMIC_STYLES
#	ENABLE_T1
#	ENABLE_MAC_T1
#	ENABLE_CFF
#	ENABLE_NON_RAM_STREAM
#	USE_NON_ZERO_WINDING_RULE
#	MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
#	REVERSE_SC_Y_ORDER
#	ENABLE_SBIT
#	ENABLE_NATIVE_TT_HINTS
#	ENABLE_AUTO_GRIDDING
#	ENABLE_T2KS
#	ENABLE_AUTO_GRIDDING_CORE
#	ENABLE_PFR
#	MY_CPU_WRITES_WORDS_SLOW
#	ENABLE_MORE_TT_COMPATIBILITY
#	ENABLE_LCD_OPTION
#	ENABLE_PCL
#	ENABLE_PCLETTO
#	ENABLE_SPD
#	ENABLE_STRKCONV	fontSize32orSmaller
#	ENABLE_FF_CURVE_CONVERSION
#	ENABLE_NATIVE_T1_HINTS
#	ENABLE_GASP_TABLE_SUPPORT
#	USE_SEAT_BELTS
#	ROM_BASED_T1

# various settings
#	exp: default desktop setting: tt, t1. cff and t2k support, full tt hinting and autogridding
#	ttfull: only tt with full hinting and autogridding
#	ttbits: only tt, but no hinting or autogridding
#	ttmin: only tt - no hinting, autogridding or embedded bitmaps


# exp: 191k/6k
ifeq ($(call MVAR_TO_TARGET_NAME, FF_CAPABILITY, exp), exp)
	MY_CFLAGS += -DENABLE_T1 -DENABLE_CFF -DENABLE_NON_RAM_STREAM -DUSE_NON_ZERO_WINDING_RULE \
		-DENABLE_SBIT -DENABLE_NATIVE_TT_HINTS -DENABLE_AUTO_GRIDDING -DENABLE_T2KS \
		-DENABLE_AUTO_GRIDDING_CORE -DENABLE_MORE_TT_COMPATIBILITY -DENABLE_STRKCONV=32 \
		-DENABLE_GASP_TABLE_SUPPORT
endif

# ttfull: 116k/332b
ifeq ($(call MVAR_TO_TARGET_NAME, FF_CAPABILITY, exp), ttfull)
	MY_CFLAGS += -DENABLE_NON_RAM_STREAM -DUSE_NON_ZERO_WINDING_RULE -DENABLE_SBIT \
		-DENABLE_NATIVE_TT_HINTS -DENABLE_AUTO_GRIDDING -DENABLE_AUTO_GRIDDING_CORE \
		-DENABLE_MORE_TT_COMPATIBILITY -DENABLE_GASP_TABLE_SUPPORT
endif

# ttbits: 64k/248b
ifeq ($(call MVAR_TO_TARGET_NAME, FF_CAPABILITY, exp), ttbits)
	MY_CFLAGS += -DENABLE_NON_RAM_STREAM -DUSE_NON_ZERO_WINDING_RULE -DENABLE_SBIT \
		-DENABLE_MORE_TT_COMPATIBILITY -DUSE_SEAT_BELTS
endif

# ttmin: 49k/248b
ifeq ($(call MVAR_TO_TARGET_NAME, FF_CAPABILITY, exp), ttmin)
	MY_CFLAGS += -DENABLE_NON_RAM_STREAM -DUSE_NON_ZERO_WINDING_RULE
endif

# ttklfull: 119k/332b
ifeq ($(call MVAR_TO_TARGET_NAME, FF_CAPABILITY, exp), ttklfull)
	MY_CFLAGS += -DENABLE_NON_RAM_STREAM -DUSE_NON_ZERO_WINDING_RULE -DENABLE_SBIT \
		-DENABLE_NATIVE_TT_HINTS -DENABLE_AUTO_GRIDDING -DENABLE_AUTO_GRIDDING_CORE \
		-DENABLE_MORE_TT_COMPATIBILITY -DENABLE_GASP_TABLE_SUPPORT \
		-DENABLE_LINE_LAYOUT -DENABLE_KERNING
endif


# ttklfullcache: 119k/332b
ifeq ($(call MVAR_TO_TARGET_NAME, FF_CAPABILITY, exp), ttklfullcache)
	MY_CFLAGS += -DENABLE_NON_RAM_STREAM -DUSE_NON_ZERO_WINDING_RULE -DENABLE_SBIT \
		-DENABLE_NATIVE_TT_HINTS -DENABLE_AUTO_GRIDDING -DENABLE_AUTO_GRIDDING_CORE \
		-DENABLE_MORE_TT_COMPATIBILITY -DENABLE_GASP_TABLE_SUPPORT \
		-DENABLE_LINE_LAYOUT -DENABLE_KERNING -DLAYOUT_CACHE_SIZE=149
endif


MY_INCLUDES := \
			-I$(BUILDHOME)/$(SRC_DIR) \
			-I$(BUILDHOME)/$(SRC_DIR)/$(FFDIR) \
#

MY_INSTALL_DIR := $(INSTALL_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := libroot.so libbe2.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

