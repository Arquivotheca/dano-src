TARGET_NAME := ati.accelerant

SRC_DIR := src/addons/accelerants/ati

SRCS := \
	Acceleration.c \
	CardStartScript.c \
	Cursor.c \
	EngineManagment.c \
	GlobalData.c \
	GetAccelerantHook.c \
	GetModeInfo.c \
	InitAccelerant.c \
	ProposeDisplayMode.c \
	SetDisplayMode.c \
	SetRegisters.c \
	hooks_overlay.c

# XXX-jbq 2001-04-12 are these flags needed?
MY_CFLAGS := -DDEBUG=1 -DCOMPILING_ACCELERANT

MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/graphics_p \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/kit/dinky/include

SYSTEM_LIBS := libroot.so

MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/accelerants

SIGNATURE := "application/x-vnd.Be-ati.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
