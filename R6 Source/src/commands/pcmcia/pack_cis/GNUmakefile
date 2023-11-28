# Makefile
TARGET_NAME:=pack_cis
SRC_DIR:=src/commands/pcmcia/pack_cis

SRCS:=pack_cis.c yacc_cis.y lex_cis.l
MY_CFLAGS:=
MY_INCLUDES:=-I$(BUILDHOME)/src/inc/driver -I$(OBJ_DIR_FROM_SRC_DIR)
RESOURCE_FORK:=
LINK_DIRS:=
SYSTEM_LIBS:=libroot.so

MY_INSTALL_DIR:=$(INSTALL_BIN)

include $(SDIR_TEMPLATES)/AppTemplate.mk


