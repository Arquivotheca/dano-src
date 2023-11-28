
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe2/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/interface2

MY_CFLAGS:= $(F_NO_EXCEPTION)

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc 

ifeq ($(COMPILER), metrowerks)
MY_INCLUDES += \
		-I$(BUILDHOME)/metro/cpp-inc \
		-I$(BUILDHOME)/src/nukernel/cpu/$(CPU)
else
MY_INCLUDES	+= -I$(BETOOLSHOME)/include/g++
endif
			
SRCS :=  \
	InterfaceUtils.cpp \
	Static.cpp \
	Surface.cpp \
	SurfaceManager.cpp \
	ViewList.cpp \
	ViewParent.cpp \
	View.cpp \
	ViewLayout.cpp \
	ViewGroup.cpp \
	ViewLayoutRoot.cpp \
	ViewHVGroup.cpp \
#
		
include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
