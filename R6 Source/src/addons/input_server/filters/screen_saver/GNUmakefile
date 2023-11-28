TARGET_NAME:=screen_saver

SRC_DIR:=src/addons/input_server/filters/screen_saver

SRCS	:= ScreenSaverController.cpp ScreenSaverFilter.cpp ScreenSaverControllerNode.cpp

MY_CFLAGS := \
			$(call MFLAG_TO_DFLAG, SCREENSAVER_LAUNCH_BY_PATH) \
			$(call MFLAG_TO_DFLAG, SCREENSAVER_USE_BINDER) \
#

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/preferences/screensaver/screenSaverCommon \
	-I$(BUILDHOME)/headers/add-ons/input_server

SYSTEM_LIBS:= libroot.so libbe.so screensaverlib.a libexperimental.a

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/filters

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
