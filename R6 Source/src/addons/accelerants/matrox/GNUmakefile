TARGET_NAME:=matrox.accelerant

SRC_DIR:=src/addons/accelerants/matrox

SRCS		:= \
	cardinit.c \
	entrypoint.c \
	hooks_2d.c \
	hooks_cursor.c \
	hooks_dpms.c \
	hooks_init.c \
	hooks_mode.c \
	hooks_overlay.c \
	hooks_sync.c \
	mga_gx00.c \
	mga_millennium.c \
	mga_mystique.c \
	mga_util.c \
	standard_modes.c

MY_CFLAGS := -DCOMPILING_ACCELERANT

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/graphics_p/matrox \
	-I$(BUILDHOME)/src/kit/surface/include \
	-I$(BUILDHOME)/src/kit/dinky/include \
	-I$(BUILDHOME)/src/inc/graphics_p
	
SYSTEM_LIBS:=libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE:= "application/x-vnd.Be-matrox.accelerant"

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
