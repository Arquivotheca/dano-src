TARGET_NAME:=null.accelerant

SRC_DIR:=src/addons/accelerants/null

SRCS		:= \
	Acceleration.c \
	Cursor.c \
	EngineManagment.c \
	GetAccelerantHook.c \
	GetModeInfo.c \
	GlobalData.c \
	InitAccelerant.c \
	ProposeDisplayMode.c \
	SetDisplayMode.c

MY_CFLAGS := -DDEBUG=1  -D_BUILDING_accelerant=1

MY_INCLUDES:= \
	-I$(BUILDHOME)/$(SRC_DIR) \
	-I$(BUILDHOME)/src/inc/graphics_p/null

SYSTEM_LIBS:=libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE:= "application/x-vnd.Be-null.accelerant"

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
