TARGET_NAME:=Background-B

SRC_DIR:=src/addons/tracker/background

SRCS := \
		BackgroundAddOn.cpp \
		MockupView.cpp \
		PreviewView.cpp \
		Scale.cpp \
		Settings.cpp \
		SetupView.cpp \
		TrackerDefs.cpp \
		ImagePanel.cpp \
		DirPanel.cpp \
#

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/interface_p

SYSTEM_LIBS:=libroot.so libbe.so libtracker.so \
	libtranslation.so

HAS_MAIN:=true

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/Tracker

RESOURCES:=Background.rsrc

include $(SDIR_TEMPLATES)/AddonTemplate.mk
