
TARGET_NAME:= screen_blanker
SRC_DIR := src/commands/be/$(TARGET_NAME)

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := \
	Activity.cpp \
	BlankWindow.cpp \
	LockWindow.cpp \
	ModuleRunner.cpp \
	main.cpp \
#

MY_CFLAGS += $(call MFLAG_TO_DFLAG, SCREENSAVER_USE_BINDER) \
	$(call MFLAG_TO_DFLAG, SCREENBLANKER_DONT_USE_DIRECTWINDOW) \
#

MY_INCLUDES	:= \
	-I$(BUILDHOME)/headers/add-ons/screen_saver \
	-I$(BUILDHOME)/src/preferences/screensaver/screenSaverCommon
#

SYSTEM_LIBS:= libbe.so libroot.so libgame.so \
	libsocket.so libbind.so screensaverlib.a
#

RESOURCES := module_runner.rsrc

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
