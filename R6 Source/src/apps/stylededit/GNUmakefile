TARGET_NAME:=StyledEdit
MY_INSTALL_DIR:=$(INSTALL_APPS)

SRC_DIR:= src/apps/stylededit
SRCS :=  					\
	main.cpp 				\
	CStyledEditApp.cpp 		\
	CStyledEditWindow.cpp 	\
	CTextView.cpp 			\
	modal.cpp				\
    CFontMenuItem.cpp 		\
	CColorMenuItem.cpp		\
#	BottomlineInput.cpp		\
#	mappings.cpp			\
#	RK2bun.c				\
#	RKbits.c				\
#	RKcontext.c				\
#	RKdd.c					\
#	RKdic.c					\
#	RKdicsw.c				\
#	RKfq.c					\
#	RKkana.c				\
#	RKncache.c				\
#	RKngram.c				\
#	RKnheap.c				\
#	RKnword.c				\
#	RKpermdic.c				\
#	RKroma.c				\
#	RKtempdic.c				\
#	RKutil.c

MY_CFLAGS := $(call MFLAG_TO_DFLAG, ALLOW_WRITE_TYPE_ATTR)
MY_INCLUDES := -I$(BUILDHOME)/src/tracker

ifneq ($(call MFLAG_IS_SET, STYLED_EDIT_WONT_WRITE_TYPE_ATTR),)
MY_CFLAGS += -DDONT_WRITE_TYPE_ATTR=1
endif

SYSTEM_LIBS:= libroot.so libbe.so libtextencoding.so libtracker.so

RESOURCE_FORK:= $(TARGET_NAME).rsrc

LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)


include $(SDIR_TEMPLATES)/AppTemplate.mk
