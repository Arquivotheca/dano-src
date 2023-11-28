
SRC_DIR := src/addons/translators/TIFF

TARGET_NAME := TIFFTranslator

SRCS :=  TIFFHandler.cpp TIFFMain.cpp

ADDON_HAS_MAIN	:= 1

MY_CFLAGS := -DUSE_OFFS=0 
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)/tiff)
SYSTEM_LIBS := libroot.so libbe.so libtranslation.so \
	libtiff.a

MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Translators

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, \
	tiff \
, src/addons/translators/TIFF)
