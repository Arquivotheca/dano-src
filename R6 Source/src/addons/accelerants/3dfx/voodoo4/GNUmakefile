
SRC_DIR:=src/addons/accelerants/3dfx/common
MY_OBJ_OFFSET := 3dfx_voodoo4.accelerant.f
SRCS := \
					bena4.c \
#
TYPE := SHARED
MY_CFLAGS := -DDEBUG=1 -DCOMPILING_ACCELERANT

MY_INCLUDES:= -I$(BUILDHOME)/src/inc \
		   -I$(BUILDHOME)/src/kit/dinky/include \
		   -I$(BUILDHOME)/src/kit/surface/include

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

TARGET_NAME:=3dfx_voodoo4.accelerant

SRC_DIR:=src/addons/accelerants/3dfx/voodoo4

SRCS		:= voodoo4.c \
					thunk.c \
					accel.c \
					overlay.c \
					fifo.c \
					video.c \
					cursor.c \
					memMgrInc.c 

MY_CFLAGS := $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)

SYSTEM_LIBS:=libroot.so libbe.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE	:= "application/x-vnd.Be-voodoo4.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
