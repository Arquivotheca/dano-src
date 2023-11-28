
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe2/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/raster2

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
	GraphicsDefs.cpp \
	RasterRect.cpp \
	RasterPoint.cpp \
	RasterRegion.cpp \
	Static.cpp \
#	Bitmap.cpp \
#
		
include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
