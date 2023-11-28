TARGET_NAME:=ZipOMatic-Z

SRC_DIR:=src/addons/tracker/zipomatic

SRCS :=  \
	ZipOEngine.cpp \
	ZipOMatic.cpp \
	ZipOWindow.cpp \
	Bitmaps.cpp \
	BarberPoleView.cpp \
#

MY_CFLAGS :=  -DUSE_OFFS=0

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/interface_p

SYSTEM_LIBS:=libroot.so libbe.so libtracker.so

PARENT_IMAGE:=

HAS_MAIN:=true

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/Tracker

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=ZipOMatic.rsrc

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
