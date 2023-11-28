TARGET_NAME:=neomagic.accelerant

SRC_DIR:=src/addons/accelerants/neomagic

SRCS		:= neomagic.c \
						bena4.c \
						accel.c \
						mode.c \
						video.c \
						cursor.c \
						nm2200.c \
						nm2097.c 

MY_CFLAGS := -DCOMPILING_ACCELERANT

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc -I$(BUILDHOME)/src/kit/surface/include 

SYSTEM_LIBS:=libroot.so libbe.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE	:= "application/x-vnd.Be-neomagic.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
