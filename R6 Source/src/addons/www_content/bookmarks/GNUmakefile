# Makefile

TARGET_NAME:=bookmarks.so

SRC_DIR:=src/addons/www_content/bookmarks

SRCS := BookmarkManagerContent.cpp ../contact_manager/listview/TListView.cpp

MY_CFLAGS :=

MY_INCLUDES:=	-I${BUILDHOME}/headers/controls \
				-I$(BUILDHOME)/headers/www \
				-I${BUILDHOME}/srcx/gnu/libpng \
				-I${BUILDHOME}/src/addons/www_content/contact_manager/listview

SYSTEM_LIBS:=libpng.so libroot.so libbe.so libcolumnlistview.so \
	libwww.so libexperimental.a

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "text/x-vnd.Be.Bookmarks"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk


SRC_DIR:=src/addons/www_content/bookmarks/favorites

# $(call MVAR_CHECK_AND_SHOW_OPTIONS, USER_SETTINGS_FAVORITES)

SOURCE := $(call MVAR_TO_TARGET_NAME, USER_SETTINGS_FAVORITES, none)
TARGET := $(INSTALL_USER_SETTINGS)/Favorites
ATTRIBUTES :=
PERMISSIONS := $(PERMS_USER_DATA)

MY_DEPS:=$(call SRC_DIR_OUT_FROM_ARG, src/addons/www_content/bookmarks)/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
