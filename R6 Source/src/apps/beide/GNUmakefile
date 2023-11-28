
# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=BeIDE

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide

RESOURCES := IDE.f/Resources/about.rsrc \
			IDE.f/Resources/beideapp.rsrc \
			IDE.f/Resources/buttons.rsrc

# The versions come from the beideapp.rsrc
MY_VERSION := -app 1 5 0 f 0 -short r5 -long "BeOS Release 5, Metrowerks"`echo -n -e '\342\204\242'`" CodeWarrior"`echo -n -e '\302\251'`" for BeOS  Portions "`echo -n -e '\302\251'`"1998-1999 Be Incorporated"

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := $(patsubst $(BUILDHOME)/src/apps/beide/%, %, \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/Utils.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/ProgressStatusWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/BarberPoleView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/BarberPoleBitmaps.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/MWEditUtils.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/MThread.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/MTargeter.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/MFormatUtils.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/MFunctionParser.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/MFileUtils.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/CString.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/MEntryRefHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/MScriptUtils.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/MDeferredScriptHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/LazyScriptHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/LazyPropHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/MessengerHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/ScriptHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/Scripting.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/Coercion.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MTriangleListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MTriangleList.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MTextControlFilter.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MTextControl.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MTextView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MPictureMenuBar.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MPictureMenuField.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MIconMenuItem.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MPathPopup.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MFocusBox.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/hilitecolor/MHiliteColor.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MDLOGListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Interface/MAlert.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ide_about/MBufferPlayer.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ide_about/MAIFFPlayer.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ide_about/AboutBox.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/STE_f/STESupport.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/STE_f/STEngine.private.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/STE_f/STEngine.overrides.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/STE_f/STEngine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MPrefsStruct.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MEditorColorView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MEditorColorsPrefsView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MBoxControl.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MBoxControlChild.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MSyntaxStyleView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MSyntaxStylePrefsView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/KeyBindings/MKeyView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/KeyBindings/MKeyIcons.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MBindingWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MKeyBindingsListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MKeyBindingView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MTargetView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MTargetListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MPrefsListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MPlugInShepard.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MPreferences.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MPreferencesWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MPreferencesView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MEnvironmentPreferencesWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MProjectSettingsWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MFontView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MEditorView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MRunPrefsView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MDefaultPrefs.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MPrefsContainer.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MBuildExtras.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MAccessPathListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/MAccessPathsView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MAreaFileList.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MBuildersKeeper.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MBuildCommander.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MCompilerObj.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MCompileGenerator.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MCompile.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MCompileTool.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MEnviron.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MFileCache.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MLinkObj.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MLauncher.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MProject.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MPreCompileObj.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MProjectCompiler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/MStartBuildThread.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Build/pool_alloc.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MWRegExp.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MWMunger.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MFindThing.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MFileSet.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MSaveFileSetWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MRemoveFileSetWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MMultiFindThread.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MMultiFileListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Find/MFindWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MMessageInfoView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MMessageWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MErrorMessageWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MInformationMessageWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MLookupThreadInformationWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MMessageView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MListView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/FindDefinition/Unmangle.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/FindDefinition/MSearchBrowse.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/FindDefinition/MFindDefinitionTask.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/FindDefinition/MFindDefinitionWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/MMessageItem.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/MProjectFileHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MProjectInfoView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MPostLinkFileLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MIgnoreFileLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MSourceFileList.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MSourceFileLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MSubProjectFileLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MSourceFile.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MSectionLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MProjectLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MPCHFileLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MLibraryFileLine.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MFindFilesThread.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MChooseNameWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MBlockFile.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MProjectView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/MProjectWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MKeywordList.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MTextAddOn.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MTextAddOnStorage.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MTextInfoView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MTextUndoer.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MSyntaxStyler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MPopupMenu.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MGoToLineWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MFunctionPopup.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MIDETextView.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Text/MTextWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/KeyBindings/MIDECommandList.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/KeyBindings/MKeyFilter.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/KeyBindings/MKeyBindingManager.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MDynamicMenuHandler.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MainMenus.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MOpenSelectionTask.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MLookupDocumentationWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MLookupDocumentationTask.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/DocumentationLookup.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MNewProjectWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MAndyFeatureTask.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/MOpenSelectionWindow.cpp \
$(BUILDHOME)/src/apps/beide/IDE.f/Main/IDEApp.cpp \
)

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

ifeq ($(TARGET_PLATFORM), intel)

	#
	# On x86 we currently have our own local
	#
#	TEMP_STL_INCLUDE := -I$(BUILDHOME)/src/apps/beide/TempSTL/
	TEMP_STL_INCLUDE := -I$(BETOOLSHOME)/include/g++
else

	#
	# Leave out the TempSTL directory on PPC
	#
	TEMP_STL_INCLUDE :=
#

endif

MY_FIRST_INCLUDES := \
	$(TEMP_STL_INCLUDE) \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/FindDefinition/headers/ \
	-I$(BUILDHOME)/src/apps/beide/Plugin_API/MWEditorAddOns/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/STE_f/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/headers/ \
	-I$(BUILDHOME)/src/apps/beide/Plugin_API/MWPreferences/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Message_Window/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/KeyBindings/headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/hilitecolor/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Text/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Find/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Build/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/ProjectWindow/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Interface/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Scripting/Scripting/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Main/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/IDECommunications/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/ide_about/About_f/ \
	-I$(BUILDHOME)/src/inc/media_p/ \
	-I$(BUILDHOME)/src/inc/interface_p/ \
	-I$(BUILDHOME)/src/tracker/ \
	-I$(BUILDHOME)/src/inc/support_p/ \
	-I$(BUILDHOME)/src/inc/os_p/ \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libtracker.so libmedia.so

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE)

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/AppTemplate.mk

RELEASE_NOTES = Release_Notes_BeIDE

#
# In the BeIDE/tools directory, we link with gcc for both
# the compiler tool and linker tool
# The names given here must match the names used in the gccPlugin
# (and also MPrefsStruct.h)
#
COMPILER_LINK_NAME := gcc
LINKER_LINK_NAME := gcc_link
ARCHIVER_NAME := ar

USER_BEIDE_LOCATION := $(patsubst $(INSTALL_BASE)/%, /boot/%, $(INSTALL_BEIDE))

ifeq ($(PLATFORM), intel)

SRC_DIR:=src/apps/beide
SOURCE := /boot/develop/tools/gnupro/bin/gcc
LINKS := $(addprefix $(INSTALL_BEIDE_TOOLS)/, $(COMPILER_LINK_NAME))
MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE := /boot/develop/tools/gnupro/bin/gcc
LINKS := $(addprefix $(INSTALL_BEIDE_TOOLS)/, $(LINKER_LINK_NAME))
MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE := /bin/ar
LINKS := $(addprefix $(INSTALL_BEIDE_TOOLS)/, $(ARCHIVER_NAME))
MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

else

SRC_DIR:=src/apps/beide
SOURCE := $(USER_BEIDE_LOCATION)/tools/mwccppc
LINKS := $(addprefix $(INSTALL_BEIDE_TOOLS)/, mwcc)
MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

SRC_DIR:=src/apps/beide
SOURCE := $(USER_BEIDE_LOCATION)/tools/mwldppc
LINKS := $(addprefix $(INSTALL_BEIDE_TOOLS)/, mwld)
MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

SRC_DIR:=src/apps/beide
SOURCE := $(USER_BEIDE_LOCATION)/tools/mwdisppc
LINKS := $(addprefix $(INSTALL_BEIDE_TOOLS)/, mwdis)
MY_DEPS := $(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

SRC_DIR:=metro/tools
SOURCE:=mwcc
TARGET:=$(INSTALL_BEIDE_TOOLS)/mwccppc
PERMISSIONS:=$(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=metro/tools
SOURCE:=mwld
TARGET:=$(INSTALL_BEIDE_TOOLS)/mwldppc
PERMISSIONS:=$(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=metro/tools
SOURCE:=mwdis
TARGET:=$(INSTALL_BEIDE_TOOLS)/mwdisppc
PERMISSIONS:=$(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=metro/tools
SOURCE:=tell.r4
TARGET:=$(INSTALL_BEIDE_TOOLS)/tell
PERMISSIONS:=$(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

# Makefile using GenericGeneratedTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

TARGET_NAME := BeHeaders
MY_RULE := PRECOMPILE_HEADERS_RULE
SRC_DIR :=
MY_INSTALL_DIR := $(INSTALL_DEV_BE_HEADERS)/precompiled
include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

endif

-include $(call INCLUDE_IF_AVAILABLE, \
	MWPlugin \
	gccPlugin \
	Plugin_API \
	devtools \
	stationery \
	optional \
, src/apps/beide)

FILES := MFileUtils.cpp MThread.cpp
SRC_DIR:=src/apps/beide/IDE.f/Utilities
MY_INSTALL_DIR := $(INSTALL_BEIDE_MCLASSES)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

FILES := MFileUtils.h MThread.h MList.h
SRC_DIR:=src/apps/beide/IDE.f/Utilities/Headers
MY_INSTALL_DIR := $(INSTALL_BEIDE_MCLASSES)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

FILES := $(RELEASE_NOTES)
SRC_DIR:=src/apps/beide
MY_INSTALL_DIR := $(INSTALL_BEIDE)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk
