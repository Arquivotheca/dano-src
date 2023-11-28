# Makefile
TARGET_NAME := korean.so
SRC_DIR:= src/addons/textencoding/korean
SRCS := KREncoding.cpp

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
			"codec" "euc-kr" \
			-end									\
			-name "be:textencoding"

PROFILE_ME :=

# we so don't need these here
OVERRIDE_CPU_FEATURES_DFLAGS :=
OVERRIDE_CPU_ID :=
OVERRIDE_M_CPU :=
OVERRIDE_M_ARCH :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
