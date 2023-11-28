
SRC_DIR := src/addons/translators/STXT

TARGET_NAME := STXTTranslator

SRCS :=  STXTHandler.cpp STXTMain.cpp

ADDON_HAS_MAIN	:= 1

MY_CFLAGS := -DUSE_OFFS=0 
MY_INCLUDES := 
SYSTEM_LIBS := libroot.so libbe.so libtranslation.so 

MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Translators

include $(SDIR_TEMPLATES)/AddonTemplate.mk
