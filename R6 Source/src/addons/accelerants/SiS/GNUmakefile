TARGET_NAME:=sis.accelerant

SRC_DIR:=src/addons/accelerants/SiS

SRCS		:= accelerant.c bena4.c sisCRTC.c video_modes.c
SRCS		+= sisBlit.c sisOverlay.c
SRCS		+= sis620_Blit.c init3d.c
SRCS		+= sis630_Cursor.c sis630_Blit.c sis630_init3d.c

MY_CFLAGS		:= -DDEBUG=1 -DCOMPILING_ACCELERANT

MY_CFLAGS		+= -DOBJ_SIS5598=1
MY_CFLAGS		+= -DOBJ_SIS6326=1
MY_CFLAGS		+= -DOBJ_SIS620=1
MY_CFLAGS		+= -DOBJ_SIS630=1

MY_INCLUDES:= \
		-I$(SRC_DIR_OUT_FROM_SRC_DIR)

MY_INCLUDES 	+= -I$(BUILDHOME)/src/inc/graphics_p
MY_INCLUDES	+= -I$(BUILDHOME)/src/inc/graphics_p/SiS
MY_INCLUDES	+= -I$(BUILDHOME)/src/kit/dinky/include
MY_INCLUDES 	+= -I$(BUILDHOME)/src/kit/surface/include
MY_INCLUDES	+= -I$(BUILDHOME)/src/modules/generic/genpool/include

SYSTEM_LIBS:=libroot.so libdinky.a libsurface.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE	:= "application/x-vnd.Be-SiS.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
