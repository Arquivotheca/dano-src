# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=NetPositive

MY_VERSION 	:= -app 2 2 2 f0 -short 2.2.2 -long "2.2.2 "`echo -n -e '\302\251'`"1991-2001 Be Incorporated"

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/netpositive

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
ifneq ($(call MFLAG_IS_SET, NET_POSITIVE_FASTER_BUILD_HACK),)
SRCS := Everything.cpp
else
SRCS 		:=  \
	AnchorGlyph.cpp \
	BasicAuth.cpp \
	BeDrawPort.cpp \
	BeInput.cpp	\
	Bookmarks.cpp \
	Builder.cpp	\
	Cache.cpp \
	CGIF.cpp \
	Cookie.cpp \
	DownloadManager.cpp \
	DrawPort.cpp \
	FileSystemHTML.cpp \
	FindWindow.cpp \
	FolderWatcher.cpp \
	FontSubstitution.cpp \
	Form.cpp \
	Glyph.cpp \
	GIF.cpp \
	HashTable.cpp \
	HistoryMenu.cpp \
	HTMLDoc.cpp \
	HTMLTags.cpp \
	HTMLView.cpp \
	HTMLWindow.cpp \
	Image.cpp \
	ImageGlyph.cpp \
	InputGlyph.cpp \
	JPEG.cpp \
	JPEGDCT.cpp \
	JPEGDraw.cpp \
	JPEGScan.cpp \
	JSInterface.cpp \
	LocationWindow.cpp \
	MessageWindow.cpp \
	MIMEType.cpp \
	NetPositiveStreamIO.cpp \
	NPApp.cpp \
	ObjectGlyph.cpp \
	PageGlyph.cpp \
	Parser.cpp \
	PluginSupport.cpp \
	PrefsWindow.cpp \
	ProgressView.cpp \
	Protocol.cpp \
	SaveText.cpp \
	Selection.cpp \
	Store.cpp \
	Strings.cpp \
	TableGlyph.cpp \
	TextGlyph.cpp \
	Translation.cpp \
	UResource.cpp \
	URL.cpp \
	URLView.cpp \
	Utils.cpp
ifeq ($(call MFLAG_IS_SET, NO_SSL_SUPPORT),)
SRCS += BeSSL.cpp
endif
endif

# MY_CFLAGS - Template User provided flags.
MY_DFLAGS := -DBE=1

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := . \
	$(BUILDHOME)/src/inc/os_p \
	$(BUILDHOME)/src/kit/openssl/include \
	$(BUILDHOME)/headers/bone
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libmidi.so\
	libtracker.so libtextencoding.so libmail.so \
	libtranslation.so libgame.so libsocket.so libbind.so

ifeq ($(call MFLAG_IS_SET, NO_SSL_SUPPORT),)
SYSTEM_LIBS += libopenssl.so

ifneq ($(call MFLAG_IS_SET, SSL_STRONG_ENCRYPTION),)
RESOURCE_FORK := NetPositive.r
else
RESOURCE_FORK := NetPositive-40bit.r
MY_DFLAGS += -DEXPORT
endif

else
RESOURCE_FORK := NetPositive-nossl.r
MY_DFLAGS += -DNOSSL=1

endif

# The resource fork is dependant upon these.
MY_DEPS += \
	res/MICN_Bookmark res/MICN_NetPositive res/MICN_JPEG res/MICN_GIF res/MICN_HTML \
	res/ICON_Bookmark res/ICON_NetPositive res/ICON_JPEG res/ICON_GIF res/ICON_HTML \
	res/FileTypes res/3dbelogoicon.gif res/3dzooicon.gif res/File.gif res/Folder.gif res/HTML.gif res/Image.gif \
	res/Script.gif res/Startup.html res/Text.gif res/UpArrow.gif res/appservericon.gif res/blackbar.gif \
	res/cubeprefsicon.gif res/editdocicon.gif res/peopleicon.gif res/startup.gif res/transicon.gif res/whitebar.gif \
	res/trackericon.gif res/orbicon.gif \
	res/About.html res/About-nossl.html res/NetPositive.gif res/OpenLocation.html res/back_up.gif res/back_down.gif res/back_disabled.gif \
	res/forward_up.gif res/forward_down.gif res/forward_disabled.gif res/stop_up.gif res/stop_down.gif \
	res/stop_disabled.gif res/reload_up.gif res/reload_down.gif res/reload_disabled.gif res/home_up.gif \
	res/home_down.gif res/home_disabled.gif res/search_up.gif res/search_down.gif res/search_disabled.gif \
	res/lock.gif res/question.gif res/star.gif res/download_retry-pressed.gif res/download_retry.gif \
	res/download_stop-pressed.gif res/download_stop.gif res/menus res/openssl_button.gif res/toolbar res/AboutPlugins.html \
	res/dl_button_disabled.gif res/dl_button_up.gif res/dl_button_down.gif res/URLBG.gif


ifneq ($(call MFLAG_IS_SET, NET_POSITIVE_DEBUG_MENU),)
	MY_DFLAGS		+= -DDEBUGMENU
endif

ifneq ($(call MFLAG_IS_SET, NET_POSITIVE_SUPPORTS_LAYERS),)
	MY_DFLAGS		+= -DLAYERS
endif

ifneq ($(call MFLAG_IS_SET, NET_POSITIVE_SUPPORTS_PLUGINS),)
	MY_DFLAGS		+= -DPLUGINS
endif


# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_APPS)

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/AppTemplate.mk


SOURCE := NetPositive.h
SRC_DIR:=src/apps/netpositive
TARGET := $(INSTALL_DEV_BE_HEADERS)/be_apps/NetPositive/$(SOURCE)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
