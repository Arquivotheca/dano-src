SRC_DIR:=src/kit/font

TARGET_NAME := libfont.so

FONTDIR		= fontfusion/core

SRCS :=	\
	$(FONTDIR)/autogrid.c \
	$(FONTDIR)/fft1hint.c \
	$(FONTDIR)/fnt.c \
	$(FONTDIR)/ghints.c \
	$(FONTDIR)/glyph.c \
	$(FONTDIR)/hstrat97.c \
	$(FONTDIR)/orion.c \
	$(FONTDIR)/pclread.c \
	$(FONTDIR)/pfrread.c \
	$(FONTDIR)/scoder.c \
	$(FONTDIR)/shapet.c \
	$(FONTDIR)/sniffer.c \
	$(FONTDIR)/spdread.c \
	$(FONTDIR)/strkconv.c \
	$(FONTDIR)/t1.c \
	$(FONTDIR)/t2k.c \
	$(FONTDIR)/t2kextra.c \
	$(FONTDIR)/t2ksbit.c \
	$(FONTDIR)/t2ksc.c \
	$(FONTDIR)/t2kstrk1.c \
	$(FONTDIR)/t2kstrm.c \
	$(FONTDIR)/t2ktt.c \
	$(FONTDIR)/truetype.c \
	$(FONTDIR)/tsimem.c \
	$(FONTDIR)/util.c
	
MY_CFLAGS :=  \
	-D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 \
	$(call MFLAG_TO_DFLAG, SUPPORTS_FONTFUSION_SEAT_BELTS) \
#

MY_INCLUDES := \
			-I$(BUILDHOME)/$(SRC_DIR) \
			-I$(BUILDHOME)/$(SRC_DIR)/$(FONTDIR) \
#

MY_INSTALL_DIR := $(INSTALL_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := libroot.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

