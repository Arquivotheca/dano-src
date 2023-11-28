
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/app


MY_CFLAGS:= $(WARN_ALL) -D_BUILDING_be=1

MY_INCLUDES:= \
			-I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/inc/app_server_p \
			-I$(BUILDHOME)/src/inc/interface_p \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/print_server_p \
			-I$(BUILDHOME)/src/inc/media_p \
			-I$(BUILDHOME)/src/tracker \
			-I$(BUILDHOME)/src/inc/input_server_p \
			-I$(BUILDHOME)/src/commands/be/mountvolume \
#

SRCS :=  \
	Application.cpp \
	AppPrefs.cpp \
	Clipboard.cpp \
	Cursor.cpp \
	Handler.cpp \
	Invoker.cpp \
	Looper.cpp \
	MessageFilter.cpp \
	Message.cpp \
	MessageBody.cpp \
	MessageList.cpp \
	MessageOld.cpp \
	MessageQueue.cpp \
	MessageRunner.cpp \
	Messenger.cpp \
	MetaPositionIO.cpp \
	Roster.cpp \
	connection.cpp \
	session.cpp \
	token.cpp

# I enable the DFLAG here, as it should eventually be pulled from
# the global DFLAGS. (steven 2001-06-26)
ifneq ($(call MFLAG_IS_SET, SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING),)
	MY_CFLAGS += $(FORCE_EXCEPTIONS)
	MY_DFLAGS += -D_SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING=1
	SRCS += MimeSniffer.cpp
endif

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_REMOTE_DISPLAY),)
	SRCS	+= TCPIPSocket.cpp
	MY_CFLAGS	+= -DREMOTE_DISPLAY
endif

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_SYNC_CALL_LOGGING),)
	MY_CFLAGS	+= -DSYNC_CALL_LOG
endif

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif

