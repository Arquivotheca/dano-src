
SRC_DIR := src/addons/translators/PPM

TARGET_NAME := PPMTranslator

SRCS :=  PPMTranslator.cpp PPMMain.cpp colorspace.cpp

ADDON_HAS_MAIN	:= 1

MY_CFLAGS := -DUSE_OFFS=0 
MY_INCLUDES := 
SYSTEM_LIBS := libroot.so libbe.so libtranslation.so 

MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Translators

include $(SDIR_TEMPLATES)/AddonTemplate.mk
