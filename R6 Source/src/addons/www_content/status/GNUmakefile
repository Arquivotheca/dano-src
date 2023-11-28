# GNUmakefile for textstatus.so
######################################################################

TARGET_NAME:=textstatus.so

SRC_DIR:=src/addons/www_content/status

MY_OBJ_OFFSET := $(TARGET_NAME).f

SRCS := TextStatusContentInstance.cpp	\
		TextStatusContent.cpp
#

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.TextStatus"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

######################################################################
# end of GNUmakefile for textstatus

# GNUmakefile for textclock.so
######################################################################

TARGET_NAME:=textclock.so

SRC_DIR:=src/addons/www_content/status

MY_OBJ_OFFSET := $(TARGET_NAME).f

SRCS := TextStatusContentInstance.cpp	\
		TextClockContentInstance.cpp	\
		TextClockContent.cpp
#

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.TextClock"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

######################################################################
# end of GNUmakefile for textclock
