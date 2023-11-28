#makefile for jpeg translation kit add-on
#
SRC_DIR := src/addons/translators/JPEG

TARGET_NAME := JPEGTranslator

SRCS :=  translator.cpp settings.cpp libjpeg_glue.cpp

MY_CFLAGS := -DUSE_OFFS=0 
MY_INCLUDES := -I$(BUILDHOME)/srcx/gnu/jpeg-6a
SYSTEM_LIBS := libjpeg.a libroot.so libbe.so libtranslation.so 
PARENT_IMAGE :=
HAS_MAIN := true
MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Translators
LINK_DIRS :=
SIGNATURE :=
RESOURCES :=
ATTRIBUTES :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

debug:
	@echo $(TARGET)
