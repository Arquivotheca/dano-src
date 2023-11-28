# Makefile
TARGET_NAME := classical.so
SRC_DIR:= src/addons/textencoding/classical
SRCS := ClassicalEncodings.cpp

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
			"codec" "iso6" \
			"codec" "iso7" \
			"codec" "iso8" \
			"codec" "iso9" \
			"codec" "cp1253" \
			"codec" "cp1254" \
			"codec" "cp1255" \
			"codec" "cp1256" \
			"codec" "cp737" \
			"codec" "cp856" \
			"codec" "cp857" \
			"codec" "cp862" \
			"codec" "cp864" \
			"codec" "cp869" \
			"codec" "MacGreek" \
			"codec" "MacHebrew" \
			"codec" "MacTurkish" \
			-end									\
			-name "be:textencoding"

PROFILE_ME :=

# we so don't need these here
OVERRIDE_CPU_FEATURES_DFLAGS :=
OVERRIDE_CPU_ID :=
OVERRIDE_M_CPU :=
OVERRIDE_M_ARCH :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
