TARGET_NAME:=chinese

SRC_DIR:=src/addons/input_server/methods/$(TARGET_NAME)

SRCS := Chinese.cpp

MY_CFLAGS := 

MY_INCLUDES:= \
				-I$(BUILDHOME)/headers/add-ons/input_server

SYSTEM_LIBS:= libroot.so libbe.so

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/methods

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=cjdict
TARGET:=$(INSTALL_USER_SETTINGS)/chinese-input/cjdict
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=phdict
TARGET:=$(INSTALL_USER_SETTINGS)/chinese-input/phdict
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=pydict
TARGET:=$(INSTALL_USER_SETTINGS)/chinese-input/pydict
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
FILES:=chinese.cfg.traditional chinese.cfg.simplified
MY_INSTALL_DIR:=$(INSTALL_USER_SETTINGS)/chinese-input
PERMISSIONS:=0644
include $(SDIR_TEMPLATES)/DataTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:= chinese.cfg.traditional
TARGET:=$(INSTALL_USER_SETTINGS)/chinese-input/chinese.cfg
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
