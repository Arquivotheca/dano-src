
include $(call INCLUDE_IF_AVAILABLE, \
	bae \
	bitflinger \
	controls \
	dinky \
	devel \
	device \
	experimental \
	fake \
	font \
	game \
	glibc \
	glue \
	jpegread \
	libbe \
	libroot \
	mail \
	media_asm \
	midi \
	midi2 \
	net \
	nuopengl \
	opengl \
	openssl \
	pcsc \
	prefpanel \
	reseditor \
	resourcehelpers \
	resparser \
	screensaver \
	settings \
	surface \
	tcpmessage \
	textencoding \
	translation \
	udpmessage \
	uncrush \
	www \
	xml \
, src/kit)

ifeq ($(call MFLAG_IS_SET,USE_IAD_MEDIA_KIT_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	media \
, src/kit)
else
include $(call INCLUDE_IF_AVAILABLE, \
	media_iad \
, src/kit)
endif

SRC_DIR := src/kit
SOURCE := libbe.so.patch.$(TARGET_CPU)
ATTRIBUTES :=
PERMISSIONS := $(PERMS_SYSTEM_DATA)
TARGET := $(INSTALL_LIBS)/$(subst .$(TARGET_CPU),,$(SOURCE))
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR := src/kit
SOURCE := libroot.so.patch.$(TARGET_CPU)
ATTRIBUTES :=
PERMISSIONS := $(PERMS_SYSTEM_DATA)
TARGET := $(INSTALL_LIBS)/$(subst .$(TARGET_CPU),,$(SOURCE))
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

