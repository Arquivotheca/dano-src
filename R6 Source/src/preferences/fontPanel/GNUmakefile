TARGET_NAME:=Fonts
MY_INSTALL_DIR:=$(INSTALL_B_BEOS_PREFERENCES_DIRECTORY)

SRC_DIR:= src/preferences/fontPanel
SRCS:= FontPanel.cpp FontOverview.cpp

SYSTEM_LIBS:= libtextencoding.so libbe.so libroot.so

RESOURCE_FORK:= FontPanel.rsrc

LINK_DIRS := $(INSTALL_USER_PREFMENU) $(INSTALL_B_PREFERENCES_DIRECTORY)

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/interface_p -I$(BUILDHOME)/src/inc/app_server_p
MY_CFLAGS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
