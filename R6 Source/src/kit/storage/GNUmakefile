
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/storage

MY_CFLAGS:= $(F_NO_EXCEPTIONS) -D_BUILDING_be=1

MY_INCLUDES := \
			-I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/nukernel/inc \
			-I$(BUILDHOME)/src/inc/interface_p \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/kit/storage
			
SRCS :=  \
		AddOnManager.cpp \
		AppFileInfo.cpp \
		DeviceMap.cpp \
		Directory.cpp \
		Entry.cpp \
		EntryRef.cpp \
		FindDirectory.cpp \
		File.cpp \
		Mime.cpp \
		Node.cpp \
		NodeInfo.cpp \
		NodeRef.cpp \
		Path.cpp \
		Query.cpp \
		QueryStack.cpp \
		QueryTree.cpp \
		Alias.cpp \
		Resources.cpp \
		ResourceStrings.cpp \
		SearchPath.cpp \
		Statable.cpp \
		SymLink.cpp \
		Volume.cpp \
		VolumeRoster.cpp \
		Walker.cpp \
		write_res.c
		

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif

