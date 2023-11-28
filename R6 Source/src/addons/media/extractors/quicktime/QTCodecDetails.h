// QTCodecDetails.h by Simon Clarke

#ifndef __QTCODECDETAILS_H
#define __QTCODECDETAILS_H

class SMCodecLink;
class BMediaNode;

#include <GraphicsDefs.h>

// entry per codec

struct audio_smp_details {

	uint32				codecID;
	uint32				codecVersion, codecRevision, codecVendor;
	void				*codecData; 
	size_t				codecDataLength;

	SMCodecLink			*codecLink;
	BMediaNode			*sysCodec;
	
	void				*codecLocalData;

	uint32				sampleDesc;

	// audio specific

	int32				bitsPerSample;
	int32				audioChannels;
	int32				audioFlags;
	int32				audioPackSize;
	int32				audioSampleRate;
	int32				bytesPerFrame;

};

// video codec storage

struct video_smp_details {

	uint32				codecID;
	uint32				codecVersion, codecRevision, codecVendor;
	void				*codecData; 
	size_t				codecDataLength;

	SMCodecLink			*codecLink;
	BMediaNode			*sysCodec;
	void				*codecLocalData;

	uint32				sampleDesc;

	// video specific
	uint32				temporalQuality, spatialQuality;
	uint32				width, height, horRes, vertRes;
	uint32				depth, flags;
	float				usecs_per_frame;   /* the stts chunk may override */
	rgb_color			*colourMap;
	uint32				colourCount;
	
};

#endif
