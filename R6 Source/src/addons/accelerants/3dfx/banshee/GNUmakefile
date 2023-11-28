
SRC_DIR:=src/addons/accelerants/3dfx/common
MY_OBJ_OFFSET := 3dfx_banshee.accelerant.f
SRCS := \
					bena4.c \
					I2CBus.cpp \
#
TYPE := SHARED
MY_CFLAGS := -DDEBUG=1 -DCOMPILING_ACCELERANT

MY_INCLUDES:= -I$(BUILDHOME)/src/inc \
		   -I$(BUILDHOME)/src/kit/dinky/include \
		   -I$(BUILDHOME)/src/kit/surface/include

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

TARGET_NAME:=3dfx_banshee.accelerant

SRC_DIR:=src/addons/accelerants/3dfx/banshee

MY_CFLAGS := $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)

SRCS		:= banshee.c \
					thunk.c \
					accel.c \
					thdfxI2C.cpp \
					tvout.cpp \
					overlay.c \
					fifo.c \
					video.c \
					cursor.c \
					memMgrInc.c 

SYSTEM_LIBS:=libroot.so libbe.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE:= "application/x-vnd.Be-banshee.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
