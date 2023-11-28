# Makefile
TARGET_NAME := latin.so
SRC_DIR:= src/addons/textencoding/latin
SRCS := LatinEncodings.cpp

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
			"codec" "iso1" \
			"codec" "iso2" \
			"codec" "iso3" \
			"codec" "iso15" \
			"codec" "msdos" \
			"codec" "mswin" \
			"codec" "cp1250" \
			"codec" "cp1252" \
			"codec" "cp437" \
			"codec" "cp850" \
			"codec" "cp852" \
			"codec" "cp860" \
			"codec" "unicode" \
			"codec" "MacRoman" \
			"codec" "MacCenteuro" \
			"codec" "MacExpert" \
			"codec" "AdobeStd" \
			-end									\
			-name "be:textencoding"

PROFILE_ME :=

# we so don't need these here
OVERRIDE_CPU_FEATURES_DFLAGS :=
OVERRIDE_CPU_ID :=
OVERRIDE_M_CPU :=
OVERRIDE_M_ARCH :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
