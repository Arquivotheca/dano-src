
SRC_DIR:=src/addons/accelerants/alt.software/skel
include $(SRC_DIR_OUT_FROM_SRC_DIR)/$(MAKEFILE_LEAFNAME)-accel_srcs
SRCS := $(GDS_ACC_SRCS)
TYPE := SHARED
MY_OBJ_OFFSET := rage128

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc/graphics_p/alt.software/skel \
		-I$(call FULL_SRC_DIR, src/addons/accelerants/alt.software)/skel \
		-I$(BUILDHOME)/src/inc/graphics_p/rage128 \
		-I$(BUILDHOME)/headers \
		-I$(BUILDHOME)/headers/add-ons/graphics \
		-I$(BUILDHOME)/headers/interface \
		-I$(BUILDHOME)/headers/device \
		-I$(BUILDHOME)/headers/drivers \
		-I$(BUILDHOME)/headers/kernel \
		-I$(BUILDHOME)/headers/posix \
		-I$(BUILDHOME)/headers/support \
		-I$(BUILDHOME)/headers/storage \
		-I$(GCCHOME)/include

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

TARGET_NAME:=rage128.accelerant

SRC_DIR:=src/addons/accelerants/rage128


SRCS		:= \
          r128_2d.c              \
          r128_cursor.c          \
          r128_accel_init.c      \
          r128_modeset.c         \
		  regdump.c \
#

MY_CFLAGS := -DDEBUG=1

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc/graphics_p/alt.software/skel \
		-I$(call FULL_SRC_DIR, src/addons/accelerants/alt.software)/skel \
		-I$(BUILDHOME)/src/inc/graphics_p/rage128 \
		-I$(BUILDHOME)/headers \
		-I$(BUILDHOME)/headers/add-ons/graphics \
		-I$(BUILDHOME)/headers/interface \
		-I$(BUILDHOME)/headers/device \
		-I$(BUILDHOME)/headers/drivers \
		-I$(BUILDHOME)/headers/kernel \
		-I$(BUILDHOME)/headers/posix \
		-I$(BUILDHOME)/headers/support \
		-I$(BUILDHOME)/headers/storage \
		-I$(GCCHOME)/include

SYSTEM_LIBS:=libroot.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

SIGNATURE	:= "application/x-vnd.Be.rage128.accelerant"

include $(SDIR_TEMPLATES)/AddonTemplate.mk
