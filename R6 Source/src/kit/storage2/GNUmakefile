
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe2/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/storage2

MY_CFLAGS:= $(F_NO_EXCEPTIONS)

MY_INCLUDES := \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/storage2_p \
#
			
SRCS :=  \
		AddOnManager.cpp \
		Directory.cpp \
		Entry.cpp \
		EntryRef.cpp \
		FindDirectory.cpp \
		File.cpp \
		Node.cpp \
		NodeRef.cpp \
		Path.cpp \
		SearchPath.cpp \
		Statable.cpp \
		SymLink.cpp \
		Volume.cpp \
#
#		write_res.c \
#		Walker.cpp \
#		Resources.cpp \
#		ResourceStrings.cpp \
#		Alias.cpp \
#		Query.cpp \
#		QueryStack.cpp \
#		QueryTree.cpp \
#		NodeInfo.cpp \
#		Mime.cpp \
#		DeviceMap.cpp \
#		VolumeRoster.cpp \
#		AppFileInfo.cpp \

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif

