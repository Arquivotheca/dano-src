
TARGET_NAME:= bootman
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := \
	install.cpp \
	intel_partition/intel_types.c \
	uninstall.cpp \
	widget.cpp \
	window.cpp \
	main.cpp

MY_CFLAGS:= -Dxor=XOR -Dor=OR -Dand=AND

MY_INCLUDES += \
			-I$(BUILDHOME)/src/inc/driver \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/addons/drive_setup/partition/intel

SYSTEM_LIBS:= libbe.so libroot.so libtracker.so

LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
