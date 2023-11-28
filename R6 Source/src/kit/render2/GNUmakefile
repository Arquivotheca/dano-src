
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe2/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/render2

MY_CFLAGS:= $(F_NO_EXCEPTION)

MY_INCLUDES:= \
		-I$(BUILDHOME)/src/inc 

ifeq ($(COMPILER), metrowerks)
MY_INCLUDES += \
		-I$(BUILDHOME)/metro/cpp-inc \
		-I$(BUILDHOME)/src/nukernel/cpu/$(CPU) \
else
MY_INCLUDES	+= -I$(BETOOLSHOME)/include/g++
endif
			
SRCS :=  \
	2dTransform.cpp \
	Bezier.cpp \
	Color.cpp \
	Stroke.cpp \
	Path.cpp \
	Pixmap.cpp \
	Point.cpp \
	Rect.cpp \
	Region.cpp \
	Render.cpp \
	RenderInputPipe.cpp \
	RenderOutputPipe.cpp \
	RenderState.cpp \
	Update.cpp \
	FontEngine.cpp \
	Font.cpp \
	Edges.cpp \
#
		
include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif
