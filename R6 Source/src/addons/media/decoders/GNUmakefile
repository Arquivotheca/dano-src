# Makefile

include $(call INCLUDE_IF_AVAILABLE, \
	applevideo \
	ccittadpcm \
	cinepak \
	dv \
	dvaudio \
	ima4 \
	indeo5 \
	msadpcm \
	msrle \
	msvideo \
	pjpeg \
	raw \
	rgb \
	ulaw \
	yuv \
, src/addons/media/decoders)

ifeq ($(call MFLAG_IS_SET,USE_IAD_MP3_DECODER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	mpegaudio_fh \
, src/addons/media/decoders)
else
include $(call INCLUDE_IF_AVAILABLE, \
	mpegaudio_fh_iad \
, src/addons/media/decoders)
endif

ifeq ($(call MFLAG_IS_SET,USE_IAD_MPEG1VIDEO_DECODER_SOURCES),)
include $(call INCLUDE_IF_AVAILABLE, \
	mpeg1video \
, src/addons/media/decoders)
else
include $(call INCLUDE_IF_AVAILABLE, \
	mpeg1video_iad \
, src/addons/media/decoders)
endif

