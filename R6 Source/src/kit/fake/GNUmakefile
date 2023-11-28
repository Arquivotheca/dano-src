SRC_DIR:=src/kit/fake
TYPE:=FAKESHARED
TARGET_NAME := librootfake.so

SRCS := \
		librootfake.c \
#

MY_CFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1


MY_INSTALL_DIR := $(INSTALL_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := 

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)
MY_OPTIMIZER:=$(DEFAULT_OPTIMIZER)
include $(SDIR_TEMPLATES)/ComplexTemplate.mk

