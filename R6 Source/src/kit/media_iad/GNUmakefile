SRC_DIR:=src/kit/media_iad

TARGET_NAME := libmedia.so

MY_CFLAGS :=  --include $(BUILDHOME)/$(SRC_DIR)/prefix.h -fno-pic -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

# MFILE_SRC
SRCS := \
	addons.cpp \
	Decoder.cpp \
	Detractor.cpp \
	Encoder.cpp \
	Extractor.cpp \
	InfoStructs.cpp \
	MediaFile.cpp \
	MediaDecoder.cpp \
	MediaEncoder.cpp \
	MediaFormats.cpp \
	MediaTrack.cpp \
	MediaWriter.cpp \
	PlaySound.cpp \
	TrackReader.cpp \
	Utilities.cpp

ifneq ($(call MFLAG_IS_SET, SUPPORTS_MEDIA_NODES),)

# NODE_SRC
SRCS += \
	Buffer.cpp \
	BufferConsumer.cpp \
	BufferGroup.cpp \
	BufferProducer.cpp \
	Controllable.cpp \
	DigitData.cpp \
	DigitView.cpp \
	FileInterface.cpp \
	MediaAddOn.cpp \
	MediaEventLooper.cpp \
	MediaFiles.cpp \
	MediaNode.cpp \
	MediaRoster.cpp \
	MediaTheme.cpp \
	ParameterWeb.cpp \
	Sound.cpp \
	SoundPlayer.cpp \
	SoundPlayNode.cpp \
	SoundRecordNode.cpp \
	TimedEventQueue.cpp \
	TimeSource.cpp \
	Timecode.cpp \
	TVChannelControl.cpp \
	Widgets.cpp \
	_BMSystemTheme.cpp \
	rt_alloc.c \
	tr_debug.cpp \
	media_reg.cpp

# LEGACY_SRC
SRCS += \
	AbstractStream.cpp \
	AudioStream.cpp \
	Subscriber.cpp \
	Stream.cpp \
	Beeper.cpp \
	SoundFile.cpp \
	StreamController.cpp \
	AIFFSoundFile.cpp \
	UnixSoundFile.cpp \
	WAVSoundFile.cpp

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

MY_INCLUDES += -I$(BETOOLSHOME)/include/g++

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

