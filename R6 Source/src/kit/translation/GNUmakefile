SRC_DIR:=src/kit/translation

TARGET_NAME := libtranslation.so

SRCS :=  \
	TranslatorRoster.cpp \
	BitmapStream.cpp \
	TranslationUtils.cpp \
	Translator.cpp \
	WebTranslation.cpp

MY_CFLAGS := $(FORCE_EXCEPTIONS) -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := -I$(BUILDHOME)/headers/translation \
				-I$(BUILDHOME)/src/inc/support_p \
				-I$(BUILDHOME)/src/inc/translation_p

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

