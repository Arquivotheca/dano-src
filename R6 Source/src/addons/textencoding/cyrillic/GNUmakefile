# Makefile
TARGET_NAME := cyrillic.so
SRC_DIR:= src/addons/textencoding/cyrillic
SRCS := CyrillicEncodings.cpp

MY_CFLAGS :=

MY_INCLUDES := 

MY_OPTIMIZER := -O3

SYSTEM_LIBS := libroot.so libtextencoding.so

PARENT_IMAGE :=
HAS_MAIN:=

MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/textencoding

LINK_DIRS :=
SIGNATURE :=
RESOURCES :=

ATTRIBUTES := \
-complex raw -t time 0 -t message		\
			"codec" "iso5" \
			"codec" "iso13" \
			"codec" "cp1251" \
			"codec" "cp1257" \
			"codec" "cp775" \
			"codec" "cp855" \
			"codec" "cp866" \
			"codec" "MacCroatian" \
			"codec" "MacCyrillic" \
			"codec" "KOI8-R" \
			-end									\
			-name "be:textencoding"

PROFILE_ME :=

# we so don't need these here
OVERRIDE_CPU_FEATURES_DFLAGS :=
OVERRIDE_CPU_ID :=
OVERRIDE_M_CPU :=
OVERRIDE_M_ARCH :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
