SRC_DIR:=src/kit/bae

TARGET_NAME := libbae.so

SRCS :=  \
	BAE.cpp BAEFile.cpp BAEMidiInput.cpp BAE_API_BeOS.cpp \
	DriverTools.c GenChorus.c GenFiltersReverb.c GenFiltersReverbFloat.c \
	GenFiltersReverbU3232.c GenInterp2Reverb.c GenInterp2ReverbFloat.c \
	GenInterp2ReverbU3232.c GenModFiles.c GenOutput.c GenPatch.c GenReverb.c \
	GenReverbNew.c GenSample.c GenSeq.c GenSetup.c GenSong.c GenSoundFiles.c \
	GenSynth.c GenSynthDropSample.c GenSynthFilters.c GenSynthFiltersFloat.c \
	GenSynthFiltersU3232.c GenSynthInterp1.c GenSynthInterp2.c \
	GenSynthInterp2Float.c GenSynthInterp2U3232.c NewNewLZSS.c \
	SMOD_Volume_Scaler.c SampleTools.c X_API.c X_Decompress.c X_EditorTools.c \
	X_IMA.c X_Instruments.c g711.c g721.c g723_24.c g723_40.c g72x.c \
	glue.cpp

MY_CFLAGS := -DX_PLATFORM=X_BE -x c++ -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
					-I$(BUILDHOME)/$(SRC_DIR)

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so libmedia.so

LINKS:= $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

#### Data files
SOURCE := Patches.hsb
SRC_DIR := src/kit/bae
TARGET := $(INSTALL_B_BEOS_ETC_DIRECTORY)/synth/Patches.hsb
ATTRIBUTES :=
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := Patches111.hsb
SRC_DIR := src/kit/bae
TARGET := $(INSTALL_B_BEOS_ETC_DIRECTORY)/synth/Patches111.hsb
ATTRIBUTES :=
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE := Patches300.hsb
SRC_DIR := src/kit/bae
TARGET := $(INSTALL_B_BEOS_ETC_DIRECTORY)/synth/Patches300.hsb
ATTRIBUTES :=
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

