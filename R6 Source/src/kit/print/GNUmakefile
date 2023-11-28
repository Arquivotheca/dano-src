
# This little bit of cruft causes libbe to be compiled,
# if we try to compile from this directory.
ifeq ($(COMPLEX_TARGET),)
include ../libbe/${MAKEFILE_LEAFNAME}
else

SRC_DIR:= src/kit/print

MY_CFLAGS:= $(F_NO_EXCEPTION) -D_BUILDING_be=1

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/print_server_p \
				-I$(BUILDHOME)/src/inc/print_p \
				-I$(BUILDHOME)/src/inc \
				-I$(BUILDHOME)/src/kit/interface \
				-I$(BUILDHOME)/$(SRC_DIR)

ifeq ($(COMPILER), metrowerks)
MY_INCLUDES += \
		-I$(BUILDHOME)/metro/cpp-inc \
		-I$(BUILDHOME)/src/nukernel/cpu/$(CPU)
else
MY_INCLUDES	+= -I$(BUILDHOME)/gnupro/include/g++
endif
			

SRCS :=  \
	DirectPrintJob.cpp \
	ListAddOns.cpp \
	PrintEnv.cpp \
	PrinterAddOn.cpp \
	PrinterConfigAddOn.cpp \
	PrinterRasterAddOn.cpp \
	PrinterVectorAddOn.cpp \
	PrintStream.cpp \
	PrintJobSettings.cpp \
	TransportIO.cpp	\
	TransportAddOn.cpp	\
	NodeWatcher.cpp \
	PrintConfigView.cpp \
	PrintPaper.cpp \
	RawPrintJob.cpp \
#

MY_DFLAGS += $(call MFLAG_TO_DFLAG, PRINTING_FOR_DESKTOP)

ifneq ($(call MFLAG_IS_SET, PRINTING_FOR_DESKTOP),)

SRCS +=  \
	PrintWindows.cpp \
	PrintPanelWindow.cpp \
	PrintPanel.cpp \
	DefaultPanels.cpp  \
	BMPView.cpp

endif


include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk

endif

