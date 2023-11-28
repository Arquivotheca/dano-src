SRC_DIR:=src/kit/dinky

TARGET_NAME := libdinky.a

SRCS		:= listnode.c listnode_op.c listnode_rp.c tags.c bena4.c

# ugly hack necessary because libdinky is used in both
# kernel land and user land -- and find_thread is compiled
# differently in both worlds...	
MY_CFLAGS := -D_KERNEL_MODE=1 -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR)/include

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

SYSTEM_LIBS :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

