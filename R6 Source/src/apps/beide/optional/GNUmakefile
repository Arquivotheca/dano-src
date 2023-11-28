
-include $(call INCLUDE_IF_AVAILABLE, \
	CrossDevelopment  nasmPlugin \
, src/apps/beide/optional)


SOURCE := KeyBindings
SRC_DIR:=src/apps/beide/optional
MY_INSTALL_DIR := $(INSTALL_BEIDE_OPTIONAL)/$(SOURCE)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataDirTemplate.mk

SOURCE := rscompare
SRC_DIR:=src/apps/beide/optional
MY_INSTALL_DIR := $(INSTALL_BEIDE_OPTIONAL)/$(SOURCE)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataDirTemplate.mk
