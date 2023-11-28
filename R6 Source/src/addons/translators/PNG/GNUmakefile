
SRC_DIR := src/addons/translators/PNG

TARGET_NAME := PNGTranslator

SRCS			:= \
	AddOn.cpp \
	ConfigView.cpp \
	CycleStringView.cpp \
	PNGTranslator.cpp \
	Translator.cpp \
	data_array.cpp \
	message_archive.cpp

ADDON_HAS_MAIN := true

MY_INCLUDES := \
	-I${BUILDHOME}/srcx/gnu/libpng \
	-I${BUILDHOME}/srcx/gnu/zlib-1.1.3 \
#

SYSTEM_LIBS := libroot.so libbe.so libtranslation.so \
	libpng.so libz.a

MY_INSTALL_DIR := $(INSTALL_B_BEOS_ADDONS_DIRECTORY)/Translators

include $(SDIR_TEMPLATES)/AddonTemplate.mk
