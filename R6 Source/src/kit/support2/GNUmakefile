
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe2/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/support2

MY_CFLAGS:= $(F_NO_EXCEPTION) -O0 -g

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc \
		-I$(BUILDHOME)/src/inc/os_p \
#

MY_FIRST_INCLUDES:= \
		-I$(BUILDHOME)/headers/support2 \
#

ifeq ($(COMPILER), metrowerks)
MY_INCLUDES += \
		-I$(BUILDHOME)/metro/cpp-inc \
		-I$(BUILDHOME)/src/nukernel/cpu/$(CPU)
else
MY_INCLUDES	+= -I$(BETOOLSHOME)/include/g++
endif
			
SRCS :=  \
	Atom.cpp \
	Binder.cpp \
	RBinder.cpp \
	Bitfield.cpp \
	BlockCache.cpp \
	BufferedPipe.cpp \
	ByteStream.cpp \
	CallStack.cpp \
	Container.cpp \
	DebugLock.cpp \
	Event.cpp \
	Flattenable.cpp \
	Handler.cpp \
	HashTable.cpp \
	KernelStreams.cpp \
	Locker.cpp \
	Looper.cpp \
	MemoryStore.cpp \
	Message.cpp \
	MessageList.cpp \
	OrderedVector.cpp \
	Parcel.cpp \
	PortPipe.cpp \
	PositionIO.cpp \
	Root.cpp \
	StreamPipe.cpp \
	String.cpp \
	StringBuffer.cpp \
	StringIO.cpp \
	SupportUtils.cpp \
	Swap.cpp \
	Team.cpp \
	TextStream.cpp \
	TLS.cpp \
	TokenSpace.cpp \
	URL.cpp \
	Value.cpp \
	ValueMap.cpp \
	ValueStream.cpp \
	Vector.cpp \
	VectorIO.cpp \
#
#	BufferIO.cpp \
		
include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
