TARGET_NAME:=FileType-F

SRC_DIR:=src/addons/tracker/filetype

SRCS := FileType.cpp

MY_CFLAGS :=  -DUSE_OFFS=0 

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/interface_p \
				-I$(BUILDHOME)/src/inc/support_p \
				-I$(BUILDHOME)/src/preferences/mime \
				-I$(BUILDHOME)/src/preferences/mime/fbe \
#

SYSTEM_LIBS:=libroot.so libbe.so libtracker.so libtranslation.so \
	filetypelib.a fbelib.a

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/Tracker

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=FileType.rsrc

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
