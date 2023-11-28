# Makefile

SRC_DIR:= src/kit/libbe2/ui

SRCS := \
	GehmlBlock.cpp \
	GehmlContent.cpp \
	GehmlDocument.cpp \
	GehmlHVGroup.cpp  \
	GehmlStack.cpp \
#

MY_CFLAGS := 

MY_INCLUDES:= \
	-I$(BUILDHOME)/$(SRC_DIR) \
	-I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/headers/uicore \
	-I$(BUILDHOME)/headers/ui \
	-I$(BUILDHOME)/headers/support \
	-I$(BUILDHOME)/headers/xml \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/inc/app_p \
	-I$(BUILDHOME)/src/inc/app_server_p \
#

CC :=        $(BUILDHOME)/gnupro-000224/bin/gcc
CXX =       $(BUILDHOME)/gnupro-000224/bin/g++

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=

MY_OPTIMIZER:= -O3
include $(SDIR_TEMPLATES)/KitTemplate.mk
