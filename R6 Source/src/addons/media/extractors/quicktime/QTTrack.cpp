// QTTrack.cpp by Simon Clarke 

#include "QTTrack.h"

#include <string.h>

QTTrack::QTTrack()
{
	SetType(QT_UNKNOWN);

	fEdits = 0;
	
	fEditList = NULL;
	fTrackID = -1;
	fTrackNumber = -1;

	// indexs
//	fKeyFrameIdx = 0;
	audioIndex = 0;
	videoIndex = 0;
	encodeBufferSize = 0;
		
	fCompressionID = 0;
	
	videoItems = audioItems = 0;
	videoChunks = audioChunks = 0;
	videoChunkCount = audioChunkCount = 0;
	videoItemCount = audioItemCount = 0;
	dataSize = 0;
	videoFrameCount = 0;
	audioCodecCount = videoCodecCount = 0;
	
	audioCodecList = 0;
	videoCodecList = 0;

	swapRawAudio = false;
	
	encodeBuffer = 0;
	encodeBufferPos = 0;
	
	// timing
	initDuration = startOffset = 0;
	
	// sample table clearing
	stcoEntryCount = 0;
	stcoTable = 0;
	
	stssEntryCount = 0;
	stssTable = 0;
	stssMaxCount = 0;
	
	stszEntryCount = 0;
	stszTable = 0;
	
	stscTable = 0;
	stscEntryCount = 0;
	
	sttsTable = 0;
	sttsEntryCount = 0;

	largestEntry = 0;
		
	memset(fTrackName,0,sizeof(fTrackName));
	memset(&qt_tkhdr, 0, sizeof(qt_tkhd_details));
	memset(&qt_mdhdr, 0, sizeof(qt_mdhd_details));
	memset(&qt_mvhdr, 0, sizeof(qt_mvhd_details));
	
	startTime = forceDuration = 0;
}

QTTrack::~QTTrack()
{
	int i;

	if (fEditList) delete fEditList;
//	if (fKeyFrameIdx) free(fKeyFrameIdx);
	
	// saving information
	if (audioChunks) free(audioChunks);
	if (videoChunks) free(videoChunks);
	
	if (videoItems) free(videoItems);
	if (audioItems) free(audioItems);
	
	if (audioCodecList) {
		for(i=0; i < audioCodecCount; i++) {
			if (audioCodecList[i].codecData)
				free(audioCodecList[i].codecData);
		}
		
		free(audioCodecList);
	}

	if (videoCodecList) {
		for(i=0; i < videoCodecCount; i++) {
			if (videoCodecList[i].codecData)
				free(videoCodecList[i].codecData);

			if (videoCodecList[i].colourMap)
				free(videoCodecList[i].colourMap);
		}
		
		free(videoCodecList);
	}

	// sample table
	if (stcoTable) free(stcoTable);
	if (stssTable) free(stssTable);
	if (stszTable) free(stszTable);
	if (stscTable) free(stscTable);
	if (sttsTable) free(sttsTable);

	if (videoIndex) free(videoIndex);
	if (audioIndex) free(audioIndex);
	
	// encode buffer
	if (encodeBuffer) free(encodeBuffer);
}

void QTTrack::SetTrackID(uint32 id)
{
	fTrackID = id;
}

uint32 QTTrack::TrackID()
{
	return fTrackID;
}

void QTTrack::SetTrackNumber(uint32 number)
{
	fTrackNumber = number;
}

uint32 QTTrack::TrackNumber()
{
	return fTrackNumber;
}

void *QTTrack::CompressorData()
{
	return fCompressorData;
}

void QTTrack::SetCompressorData(void *data)
{
	fCompressorData = data;
}

qt_track_type QTTrack::Type()
{
	return fType;
}

void QTTrack::SetType(qt_track_type type)
{
	fType = type;
}

char *QTTrack::Name()
{
	return fTrackName;
}

void QTTrack::SetName(char *name)
{
	strcpy(fTrackName,name);
}

bool QTTrack::HasTrackName() const
{
	if (fTrackName[0] == 0) return false;
	return true;
}

void QTTrack::SetCompressionID(uint32 compressionid)
{
	fCompressionID = compressionid;
}

uint32 QTTrack::CompressionID() const
{
	return fCompressionID;
}

void QTTrack::InitEditList(uint32 entries)
{
	fEditList = new edit_list[entries+1];
}

void QTTrack::AddEditListEntry(uint32 duration, uint32 time, uint32 rate)
{
	fEditList[fEdits].duration = duration;
	fEditList[fEdits].time = time;
	fEditList[fEdits].rate = rate;
	
	fEdits++;
}

uint32 QTTrack::CountEdits()
{
	return fEdits;
}

void QTTrack::GetEditAt(uint32 idx, uint32 &duration, uint32 &time, uint32 &rate)
{
	duration = fEditList[idx].duration;
	time = fEditList[idx].time;
	rate = fEditList[idx].rate;
}

void QTTrack::SetVIndex(MediaIndex *mediaindex)
{
	videoIndex = mediaindex;
}

MediaIndex *QTTrack::VIndex() const
{
	return videoIndex;
}

MediaIndex *QTTrack::AIndex() const
{
	return audioIndex;
}

//KeyFrameVideoIndex *QTTrack::KFIndex() const
//{
//	return fKeyFrameIdx;
//}

//void QTTrack::SetKFIndex(KeyFrameVideoIndex *idx)
//{
//	fKeyFrameIdx = idx;	
//}
