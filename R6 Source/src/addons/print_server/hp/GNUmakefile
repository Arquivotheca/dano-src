# makefile

SRC_DIR := src/addons/print_server/hp

TARGET_NAME := HP\ PCL3\ LaserJet\ Compatible

SRCS :=  hp_pcl.cpp

MY_CFLAGS := 
MY_INCLUDES := \
	-I$(BUILDHOME)/$(SRC_DIR) \
#

SYSTEM_LIBS := libroot.so libbe.so
PARENT_IMAGE :=

MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/Print

include $(SDIR_TEMPLATES)/AddonTemplate.mk
