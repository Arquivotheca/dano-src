// QTTrack.cpp by Simon Clarke

#ifndef __QTTRACK_H
#define __QTTRACK_H

#include "QTStructures.h"
#include "MediaIndex.h"

#include "QTCodecDetails.h"

typedef struct {

	uint32 	duration;
	uint32	time;
	uint32	rate;

} edit_list;

typedef struct {

	size_t 	itemsize;
	off_t	itemposition;

} ItemEntry;

typedef struct {
	
	size_t	chunkItems;
	off_t	chunkOffset;
	
} ChunkEntry;

class QTTrack {
public:

		QTTrack();
		~QTTrack();

		void InitEditList(uint32 entries);
		void AddEditListEntry(uint32 duration, uint32 time, uint32 rate);
		uint32 CountEdits();
		void GetEditAt(uint32 idx, uint32 &duration, uint32 &time, uint32 &rate);

		void SetTrackID(uint32 id);
		uint32 TrackID();
		
		void SetTrackNumber(uint32 number);
		uint32 TrackNumber();

		qt_track_type Type();
		void SetType(qt_track_type type);

		void *CompressorData();
		void SetCompressorData(void *data);
										
		char *Name();
		void SetName(char *name);
		bool HasTrackName() const;
		
		void SetCompressionID(uint32 compressionid);
		uint32 CompressionID() const;
		
		void SetVIndex(MediaIndex *mediaindex);
		MediaIndex* VIndex() const;

		MediaIndex* AIndex() const;
						
		// output details
		ItemEntry			*videoItems;
		ItemEntry 			*audioItems;
		
		ChunkEntry			*videoChunks;
		ChunkEntry			*audioChunks;
		
		uint32				videoChunkCount, audioChunkCount;
		uint32				videoItemCount, audioItemCount;

		// shared details
		size_t				dataSize;
		uint32				videoFrameCount, audioFrameCount;
		size_t				largestEntry;
		
		bigtime_t			startTime, forceDuration;
		
		// cache details
		bool				loadInfo;
		uint32				loadStart, loadDuration;
		uint32				loadDescription;
		
		// audio details
		MediaIndex			*audioIndex;
		audio_smp_details	*audioCodecList;
		uint32				audioCodecCount;
		bool				swapRawAudio;

		// video details
		uint32				videoCodecCount;
		MediaIndex			*videoIndex;
		video_smp_details	*videoCodecList;
		uint32				imageWidth, imageHeight;
		
		// sample table details
		uint32				*stcoTable; // stco
		uint32				stcoEntryCount;
		
		uint32				*stssTable; // stss
		uint32				stssEntryCount;
		uint32				stssMaxCount;
		
		uint32				*stszTable; // stsz
		uint32				stszEntryCount;

		qt_stsc_details		*stscTable;
		uint32				stscEntryCount;
		
		qt_stts_details		*sttsTable;
		uint32				sttsEntryCount;

		// timing details
		uint32				initDuration, startOffset; // from elst

		// track headers
		qt_tkhd_details		qt_tkhdr;
		qt_mdhd_details		qt_mdhdr;
		qt_mvhd_details		qt_mvhdr;

		// encoding details
		void				*encodeBuffer;
		size_t				encodeBufferPos;
		size_t				encodeBufferSize;

private:

		uint32				fEdits;
		edit_list			*fEditList;
		int32				fTrackID, fTrackNumber;
		void				*fCompressorData;
		void				*fAudioCodecData[5];
		qt_track_type		fType;
		char				fTrackName[256];
		uint32				fCompressionID;
};

#endif
