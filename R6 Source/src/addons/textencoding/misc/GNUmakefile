# Makefile
TARGET_NAME := misc.so
SRC_DIR:= src/addons/textencoding/misc
SRCS := MiscEncodings.cpp

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
			"codec" "iso4" \
			"codec" "iso10" \
			"codec" "iso14" \
			"codec" "cp1258" \
			"codec" "cp861" \
			"codec" "cp863" \
			"codec" "cp865" \
			"codec" "cp874" \
			"codec" "MacIceland" \
			"codec" "MacDingbats" \
			"codec" "AdobeSymbol" \
			"codec" "ZapfDingbats" \
			-end									\
			-name "be:textencoding"

PROFILE_ME :=

# we so don't need these here
OVERRIDE_CPU_FEATURES_DFLAGS :=
OVERRIDE_CPU_ID :=
OVERRIDE_M_CPU :=
OVERRIDE_M_ARCH :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
