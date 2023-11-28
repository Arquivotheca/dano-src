// QTAtomHandler.h by Simon Clarke (S.J.Clarke@herts.ac.uk) - Copyright 1996-7

#ifndef __QTCHUNKHANDLER_H
#define __QTCHUNKHANDLER_H

#include "QTStructures.h"
#include "MediaIndex.h"
#include "QTCodecDetails.h"

class QTTrack;

#include <SupportDefs.h>

class QTAtomHandler {
public:

		QTAtomHandler();
		~QTAtomHandler();
		
		void SetTo(BPositionIO *stream);				
						
		const char *Copyright(void);
		status_t Begin();
		status_t ParseHeader();
		status_t ParseAtoms(size_t fileLength);
		
		size_t ParseUserData(size_t currentLen);
		
		QTTrack *FindTrackByID(uint32 id);
		BList *TrackList();
		
		bigtime_t MaxDuration();
		
		qt_mvhd_details *MVHDHeader();
		qt_hdlr_details *HDLRHeader();
				
		static status_t IsQuickTime(const entry_ref &file);
		
		status_t ReadAcrossMedia(	void			*toBuffer,
									BPositionIO 	*stream,
									QTTrack			*track,
									MediaIndex		*mediaIndex,
									size_t			offsetBytes,
									size_t			readBytes);
					
		uint32			fMVTimescale;

private:

		// header reading
		void ReadMVHD(	qt_mvhd_details *header);
		bool ReadTKHD(	qt_tkhd_details *header);
		void ReadMDHD(	qt_mdhd_details *header);
		void ReadDREF();
		void ReadHDLR(	qt_hdlr_details *header, int32 len);
		void ReadELST(	QTTrack *inputTrack);
		
		status_t ReadHeader(size_t length);
		
		status_t ReadLOAD(	QTTrack *inputTrack);
		
		status_t ReadSTTS( 	size_t length, QTTrack *inputTrack);
				
		status_t ReadSTCO(	QTTrack *inputTrack);
		status_t ReadSTSZ(	size_t atomLength, QTTrack *inputTrack);

		status_t ReadSTSC(	size_t 			length, 
							uint32 			chunkOff, 
							uint32 			codecCount, 
							uint32 			codec1Num, 
							QTTrack 		*inputTrack);
		
		status_t ReadSTSS(	QTTrack *qtTrack);
		status_t ReadSTSD(	QTTrack *track, uint32 len);
		
		// video reading functions		
		status_t ReadVideoSTSD(int32 atom_size);
		status_t ReadVideoDescription(video_smp_details *videoDetails, int32 atom_size);
		
		status_t ProcessColourMap(video_smp_details *videoDetails, size_t length);
		
		status_t ReadVideoSTSD2(	video_smp_details *videoDetails);
		
		void CreateAppleColourMap(rgb_color *colourMap);
		
		// audio reading		
		status_t ReadAudioSTSD(int32 atom_size);
		status_t ReadAudioCodecHDR(audio_smp_details *audioDetails, int32 atom_size);
		
		// type of audio compression debug
		void PrintAudioType(uint32 type);

		//
		// low level stream reading		
		//
		size_t ReadChunk(void *dstbuf, size_t len);
		uint32 ReadMSB32();
		uint16 ReadMSB16();
		uint8 ReadMSB8();
		void RewindStream(size_t bytes);
		void AdvanceStream(size_t bytes);

		// create index for track
		status_t ParseIndex(QTTrack *track);
		
		// create index for correct functions
		status_t		CreateVideoIndex(QTTrack *qtTrack);
		status_t		CreateSoundIndex(QTTrack *inputTrack);

		void ReadName(int32 rlen);
		
		void CheckTempBuffer(uint32 len);
									
		char			*fCopyright;

		// central headers			
		qt_hdlr_details	fHDLRHeader;
		qt_mvhd_details	fMVHDHeader;

		BPositionIO		*inStream;
		
		size_t			fMoovSize;
		size_t			fStoredMDATSize;
				
		BList			*fTrackList;
		QTTrack			*fCurrentTrack;
		
		bool			fMDIACurrent;
		void			*fTempBuffer;
		uint8			*fRealBuffer;
		uint32			fBufferSize;
		uint32			fXPos, fYPos;

		// normal header reading		
		uint8			*fHeaderBuffer, *fHeaderPosition;
		
		// Decompressed header reading
		uint8			*fDecompressedBuffer, *fDecompressedPosition;
		size_t			fDecompressedSize;
		uint32			fCompressionType;
		
		// description of startup
		bool			fMdatFound;
		uint32			fAudioTrackCount, fVideoTrackCount, fTrackCount;
		
};

#endif
