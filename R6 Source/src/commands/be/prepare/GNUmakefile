# makefile
TARGET_NAME:= prepare
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_VALIDATE)

SRCS := prepare.cpp md5/md5_dgst.c

MY_CFLAGS:= -DDEBUG=1

MY_INCLUDES	:= -I$(BUILDHOME)/$(SRC_DIR)/md5

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

SOURCE := Make_Release_Image
SRC_DIR := src/commands/be/prepare/scripts
TARGET := $(INSTALL_B_DESKTOP_DIRECTORY)/Make_Release_Image
PERMISSIONS := $(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := $(call MVAR_TO_TARGET_NAME,VALIDATE_OK,validate.ok)
SRC_DIR := src/commands/be/prepare/scripts
TARGET := $(INSTALL_VALIDATE)/validate.ok
PERMISSIONS := $(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := Make_ValidateTool_MS
SRC_DIR := src/commands/be/prepare/scripts
TARGET := $(INSTALL_B_DESKTOP_DIRECTORY)/Make_ValidateTool_MS
PERMISSIONS := $(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := Make_DisplayTest_MS
SRC_DIR := src/commands/be/prepare/scripts
TARGET := $(INSTALL_B_DESKTOP_DIRECTORY)/Make_DisplayTest_MS
PERMISSIONS := $(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := Validate
SRC_DIR := src/commands/be/prepare/scripts
TARGET := $(INSTALL_B_DESKTOP_DIRECTORY)/Validate
PERMISSIONS := $(PERMS_SYSTEM_APP)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

ifdef BUILD_FILES_REMOVALS_SPEC

$(INSTALL_VALIDATE)/removals.txt :
	rm -f $@
	$(MKDIR) $(dir $@)
ifneq ($(FILES_REMOVALS_SPEC),)
	for i in $(FILES_REMOVALS_SPEC) ; do echo $$i >> $@ ; done 
endif
	touch $@

ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(INSTALL_VALIDATE)/removals.txt
else
$(INSTALL_RULES): $(filter $(INSTALL_VALIDATE)/removals.txt,$(FILES_SPEC))
FILES_SPEC:= $(filter-out $(INSTALL_VALIDATE)/removals.txt,$(FILES_SPEC))
endif

endif
