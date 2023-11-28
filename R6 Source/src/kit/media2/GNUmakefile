
SRC_DIR:= src/kit/media2

TARGET_NAME:=libmedia2.so

MY_INCLUDES := \
		-I$(BUILDHOME)/src/kit/media2 \
		-I$(BUILDHOME)/src/inc/media2_p \
#
MY_FIRST_INCLUDES := \
		-I$(BUILDHOME)/headers/media2 \
#

__MEDIA2_CODECS := 1
__MEDIA2_RESAMPLER := 1

SRCS := \
	AreaCloneCache.cpp \
	Buffer.cpp \
	BufferSource.cpp \
	BufferOutlet.cpp \
	BufferInlet.cpp \
	BufferGroup.cpp \
	buffer_source_impl.cpp \
	convert_format.cpp \
	DumbVector.cpp \
	MediaAddon.cpp \
	MediaCollective.cpp \
	MediaConstraint.cpp \
	MediaConstraintAlternative.cpp \
	MediaConstraintItem.cpp \
	MediaControllable.cpp \
	MediaEndpoint.cpp \
	MediaEndpointGraph.cpp \
	MediaFormat.cpp \
	MediaInput.cpp \
	MediaNode.cpp \
	MediaOutput.cpp \
	MediaPreference.cpp \
	MediaProducer.cpp \
	MediaProducerContext.cpp \
	resolve_format.cpp \
	Static.cpp \
	SoundPlayer.cpp \
	SoundPlayerNode.cpp \
	MediaExtractor.cpp \
	TimeSource.cpp \
	MediaRoster.cpp \
#

ifeq ($(__MEDIA2_CODECS),1)
SRCS += \
	codec_addons.cpp \
	Decoder.cpp \
	Detractor.cpp \
	Encoder.cpp \
	Extractor.cpp \
	FileWriter.cpp \
	MediaDecoder.cpp \
	MediaEncoder.cpp \
	MediaFile.cpp \
	MediaFormats.cpp \
	MediaTrack.cpp \
	MediaWriter.cpp \
#
endif

ifeq ($(__MEDIA2_RESAMPLER),1)
SRCS += \
	AudioResampler.cpp \
	resampler_direct.cpp \
	sincutil.cpp \
	zero.asm \
	linear.asm \
	trilinear.asm \
	cubic.asm \
	sinc.asm \
	sinc_lowpass.asm \
	global.asm \
	converter_i586.S \
	mixer_i586.S \
#
endif

MY_INSTALL_DIR:= $(INSTALL_LIBS)

OUR_CFLAGS := \
			$(call MFLAG_TO_DFLAG, SUPPORTS_ATOM_DEBUG) \
			$(call MFLAG_TO_DFLAG, SUPPORTS_LOCK_DEBUG) \
#

RESOURCES := \
#

SYSTEM_LIBS	:= libroot.so libbe2.so

LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk
