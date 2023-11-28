
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/support

MY_CFLAGS:= $(F_NO_EXCEPTION) -D_BUILDING_be=1

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc/os_p \
		-I$(BUILDHOME)/src/inc/support_p \
		-I$(BUILDHOME)/src/inc/app_p \
		-I$(BUILDHOME)/src/inc/media_p \
		-I$(BUILDHOME)/src/inc/driver \
		-I$(BUILDHOME)/src/inc 

ifeq ($(COMPILER), metrowerks)
MY_INCLUDES += \
		-I$(BUILDHOME)/metro/cpp-inc \
		-I$(BUILDHOME)/src/nukernel/cpu/$(CPU)
else
MY_INCLUDES	+= -I$(BETOOLSHOME)/include/g++
endif
			

SRCS :=  \
	BEvent.cpp \
	BInstanceAllocator.cpp \
	BLightLocker.cpp \
	Atom.cpp \
	Archivable.cpp \
	Autolock.cpp \
	Beep.cpp \
	Binder.cpp \
	BlockCache.cpp \
	BufferIO.cpp \
	compat.c \
	DataIO.cpp \
	DebugLock.cpp \
	fbc.c \
	Flattenable.cpp \
	Gehnaphore.cpp \
	GDispatcher.cpp \
	GLooper.cpp \
	GHandler.cpp \
	GMessageQueue.cpp \
	List.cpp \
	Locker.cpp \
	OrderedVector.cpp \
	SmooveD.cpp \
	ObjectList.cpp \
	ReadWriteLocker.cpp \
	RegExp.cpp \
	shutdown.cpp \
	StreamIO.cpp \
	String.cpp \
	StopWatch.cpp \
	Swap.cpp \
	TLS.cpp \
	TypedList.cpp \
	syslog.c	\
	UTF8.cpp	\
	Vector.cpp \
#
		
ifneq ($(call MFLAG_IS_SET, SUPPORTS_STREAM_IO, $(MFLAGS_SPEC) $(OVERRIDE_MFLAGS)),)
	SRCS	+= 	CallStack.cpp \
				StringIO.cpp
endif

include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif

