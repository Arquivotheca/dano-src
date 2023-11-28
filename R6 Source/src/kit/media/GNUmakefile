SRC_DIR:=src/kit/media

TARGET_NAME := libmedia.so

MY_CFLAGS :=  --include $(BUILDHOME)/$(SRC_DIR)/prefix.h -fno-pic -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

# after some testing, this can be go into the spec
ifneq ($(call MFLAG_IS_SET, DEBUG_BUILD),)
MY_DFLAGS += -DDEBUG=1
else
MY_DFLAGS += -DNDEBUG=1
endif

SYSTEM_LIBS := libroot.so libbe.so

# MFILE_SRC
SRCS := \
	Decoder.cpp \
	Detractor.cpp \
	Extractor.cpp \
	MediaFile.cpp \
	MediaTrack.cpp \
	MediaDecoder.cpp \
	MediaEncoder.cpp \
	addons.cpp

ifneq ($(call MFLAG_IS_SET, SUPPORTS_MEDIA_NODES),)

# NODE_SRC
SRCS += \
	Buffer.cpp \
	BufferConsumer.cpp \
	BufferGroup.cpp \
	BufferProducer.cpp \
	Controllable.cpp \
	FileInterface.cpp \
	Encoder.cpp \
	InfoStructs.cpp \
	LocalControllable.cpp \
	MediaAddOn.cpp \
	MediaEventLooper.cpp \
	MediaFiles.cpp \
	MediaFormats.cpp \
	MediaNode.cpp \
	MediaRecorder.cpp \
	MediaRecorderNode.cpp \
   	MediaRoster.cpp \
	MediaTheme.cpp \
	MediaWriter.cpp \
	ParameterWeb.cpp \
	Sound.cpp \
	SoundPlayer.cpp \
	SoundPlayNode.cpp \
	SoundRecordNode.cpp \
	SourceList.cpp \
	TimedEventQueue.cpp \
	TimeSource.cpp \
	Timecode.cpp \
	TrackReader.cpp \
	TVChannelControl.cpp \
	Utilities.cpp \
	_BMSystemTheme.cpp \
	rt_alloc.c \
	tr_debug.cpp

# LEGACY_SRC
SRCS += \
	AbstractStream.cpp \
	AudioStream.cpp \
	Subscriber.cpp \
	Stream.cpp \
	Beeper.cpp \
	PlaySound.cpp \
	SoundFile.cpp \
	StreamController.cpp \
	AIFFSoundFile.cpp \
	UnixSoundFile.cpp \
	WAVSoundFile.cpp

SYSTEM_LIBS += libtranslation.so libmedia_asm.a $(LIBSTDCPP_OBJECT)

else

# MINI_SRC
SRCS += \
	IADSoundPlayer.cpp \
	IADMediaRecorder.cpp \
	SourceList.cpp \
	miniplay.cpp \
	minilocker.cpp

endif

MY_INCLUDES := \
			-I$(BUILDHOME)/headers/media \
			-I$(BUILDHOME)/src/inc/media_p \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/driver \
			-I$(BUILDHOME)/src/inc/support_p

MY_INCLUDES += -I$(BUILDHOME)/gnupro/include/g++

MY_INSTALL_DIR := $(INSTALL_LIBS)

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

