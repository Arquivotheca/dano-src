
TARGET_NAME:=apple

SRC_DIR:=src/addons/drive_setup/partition/$(TARGET_NAME)

SRCS := \
	$(TARGET_NAME).cpp


MY_CFLAGS := 

MY_INCLUDES:= \
		-I$(BUILDHOME)/$(SRC_DIR) \
		-I$(BUILDHOME)/src/inc/os_p \
#

SYSTEM_LIBS:=libroot.so libbe.so

ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc3) 
SYSTEM_LIBS += libgcc.a
endif

$(call MVAR_CHECK_FINITE_OPTIONS, COMPILER_VERSION, gcc2 gcc3, gcc2) 

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/drive_setup/partition

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

