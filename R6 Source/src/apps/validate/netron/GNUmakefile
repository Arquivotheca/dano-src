
# Do the vendor2 add-on for validate

TARGET_NAME:=vendor2
SRC_DIR:=src/apps/validate/netron
SRCS := netron-vendor2.cpp
MY_INCLUDES:= \
	   -I$(BUILDHOME)/src/apps/validate \
#
SYSTEM_LIBS:= libroot.so libbe.so libmedia.so
PARENT_IMAGE:= $(VALIDATE_OBJECT)
HAS_MAIN:=
MY_INSTALL_DIR:=$(INSTALL_VALIDATE)/add-ons

MY_INCLUDES += 	-I$(BUILDHOME)/src/inc/media_p
include $(SDIR_TEMPLATES)/AddonTemplate.mk

