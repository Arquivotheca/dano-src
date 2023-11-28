
TARGET_NAME:=mwbres

SRC_DIR:=src/commands/restools/$(TARGET_NAME)

SRCS :=  \
	mwbres-parser.y main.cpp comptab.cpp mwbres.l 

MY_INCLUDES := -I. -I$(call FULL_OBJ_DIR, $(SRC_DIR))

SYSTEM_LIBS:= libroot.so libbe.so libfl.a

MY_YACC_FLAGS := -v $(BISON_CPP_OUTPUT_FLAG) -d

MY_LEX_FLAGS := $(FLEX_CPP_OUTPUT_FLAG) -l

MY_INSTALL_DIR:=$(INSTALL_BIN)

include $(SDIR_TEMPLATES)/AppTemplate.mk

# This is because the current system hates created header files.
DEPEND_ON_ME := mwbres-parser.h
SRC_DIR := $(SRC_DIR_IN)
SRCS := $(SRCS_IN)
include $(SDIR_TEMPLATES)/NoCircleDepTemplate.mk
