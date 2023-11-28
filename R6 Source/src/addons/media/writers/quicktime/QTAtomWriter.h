// QTAtomWriter.h by Simon Clarke (S.J.Clarke@herts.ac.uk) - Copyright 1996-7

#ifndef __QTATOMWRITER_H
#define __QTATOMWRITER_H

#include "QTStructures.h"
#include "QTTrack.h"
#include <List.h>
#include <MediaDefs.h>

class SMCodecLink;

#define FAKE_APPLE true

#define gSMediaName "BeOS Sound Handler"
#define gMediaName "BeOS Media Handler"
#define gDataName "BeOS Data Media Handler"

#define gAVMediaName "Apple Video Media Handler"
#define gAliasName "Apple Alias Data Handler"
#define gASMediaName "Apple Sound Media Handler"

#ifdef FAKE_APPLE
#define gVendor 'appl'
#else
#define gVendor 'beos'
#endif

const int32 VENDOR = 'beos';

const int32 MAX_FRAMES = 300;
const int32 MAX_AUDIOFRAMES = 300;
const int32 QTAH_SIZE = 8; // two int32s
const int32 QT_FLAGSSIZE = 4; // flag int32
const int32 FLUSH_SIZE = 100000; // 100k flush size
const int32 QT_INDEX_SIZE = 200;

const int32 QT_MAX_SAVE_AUDIO_TRACKS = 5;

const int32 SAMPLES_PER_CHUNK = 5;

class FileWriter;

class QTAtomWriter {
public:

		QTAtomWriter();
		~QTAtomWriter();

		void SetTo(FileWriter *stream);

		status_t Begin();
		status_t InitCheck();
		
		status_t BeginTrack(QTTrack	*inputTrack);
		status_t CloseTrack();

		status_t AddCopyright(const char *data);
		
		status_t AddAudioFrames(const void 		*inData,
								QTTrack			*inputTrack,
								size_t 			size);

		status_t AddVideoFrame(	const void 			*inData,
								QTTrack				*inputTrack,
								size_t 				fixedSize,
								media_encode_info	*info);
		
		status_t CloseMovie();
		
		status_t SetCachingScheme(uint32 scheme);
		
private:

		status_t WriteFullHeader();

		status_t WriteMVHD(qt_mvhd_details *header);
		status_t WriteTKHD(qt_tkhd_details *header);
		status_t WriteMDHD(qt_mdhd_details *header);
		
		status_t WriteHDLR(	const char 		*headerName,
							int32 			subType,
							int32 			type,
							int32 			headerNameLength);

		status_t WriteAppleHDLR(
							const char 		*headerName,
							int32 			subType,
							int32 			type,
							int32 			headerNameLength);
											
		status_t WriteVMHD();
		status_t WriteSMHD();
		status_t WriteELST(	int32 duration);
		
		status_t WriteVideoSTSD(video_smp_details 	*header,
								const char 			*codecName);
		
		status_t WriteAudioSTSD(int32 		sampleRate, 
								int32 		channelCount, 
								int32 		bitsPerSample, 
								int32 		codecType);

		status_t WriteAudioSTSZ(int32 		bytesPerFrame,
								int32		frameCount);

		// general sample table writing

		status_t WriteSTSS(	QTTrack			*track);
		status_t WriteSTSC(	int32 			numChunks, 
							ChunkEntry 		*entryList,
							int32			data_reference_index);

		status_t WriteSTSZ(	int32 			sampleCount, 
							ItemEntry 		*itemList);
		
		status_t WriteSTTS(	int32 			sampleCount, 
							int32 			duration,
							int32			frameCount);
		status_t WriteVideoSTTS(int32 		duration);
	
		status_t WriteSTCO(	int32 			chunkCount,
							ChunkEntry 		*itemList);
	
		status_t WriteDINFnDREF();

		status_t WriteCopyright(const char *copyright);

		void WriteMSB32(int32 value);
		void WriteMSB16(int16 value);
		void WriteMSB8(uint8 value);
		
		void WriteAtomHeader(int32 id, int32 length);
		
		void WriteName(	const char 	*name, 
						int32 		stringlength, 
						bool 		padString = false);	
		
		status_t UpdateSTTS(uint32 last_frame_duration, int final = false);

		void ReplaceLength(off_t offset, int32 value);
		void SetDefaults();
		void ReplaceEndTrak();
		void StartTrak(qt_tkhd_details *trakHeader);
	
		void PrintTrackCount();
	
		void CheckAudioIndex();
		void CheckVideoIndex(QTTrack *track);
	
		status_t WriteAudioTrack(QTTrack *track);
		status_t WriteVideoTrack(QTTrack *track);

		char				*fCopyright;

		// data and header stuff
		FileWriter			*fOutStream;
		BMessage			*fExtenstionMessage;

		// indexs
		ItemEntry			*videoitems;
		
		struct stts_entry {
			uint32	frame_count;
			uint32	frame_duration;
		};
		stts_entry			*fVideoSTTSentries;
		size_t				fVideoSTTSentries_size;
		int					fVideoSTTSentry_count;
		bigtime_t			fLastVideoTime;

		ChunkEntry			*cvideoitems;
		ChunkEntry			*caudioitems;
	
		uint32				fVidChunkPos;

		// counters		
		int32 				fTrackCount;
		size_t				fTotalMDATSize;
		
		// end counters for trak et al
		off_t				fSTBLPosition; 	// sample tables
		off_t				fTRAKPosition; 	// trak length
		off_t				fMDIAPosition; 	// mdia (subset of trak)
		off_t				fMINFPosition;

		BList				*fTrackList;
				
		// flush size		
		status_t			fInitStatus;
		
		uint32				fVideoIdxNumber, fAudioIdxNumber;
		uint32				fAudioTotalChunks;
		
		uint32				fFrameIdxSize, fVidChunkIdxSize;
		uint32				fAFrameIdxSize, fAudChunkIdxSize;
		
		uint32				fScheme;

		// raw audio byte swapping
		void				*fRawAudioSwapBuffer;
		size_t				fRawAudioSwapBufferLength;
};

#ifdef DEBUGSAVER
	#define DEBUGSAVE(text) printf(text)
#else
	#define DEBUGSAVE(text) (void)0	
#endif

#endif
