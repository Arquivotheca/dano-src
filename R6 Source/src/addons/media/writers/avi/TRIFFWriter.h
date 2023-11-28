//---------------------------------------------------------------------
//
//	File:	TRIFFWriter.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	RIFF File Format
//
//
//---------------------------------------------------------------------

#ifndef __TRIFFWRITER_H__
#define __TRIFFWRITER_H__

//	Includes
#include "RIFFConstants.h"
#include "RIFFTypes.h"

#include <FileWriter.h>

const char kRiffWriteMaxStreams = 2;

//	Class Definition
class TRIFFWriter
{
	public:
		//TRIFFWriter(BFile *file);
		TRIFFWriter(FileWriter *file);
		~TRIFFWriter();
		
		bool 	InitAVIFile(bool use_raw_data);
		bool 	CompleteAVIFile();
						
		status_t	Write00DCChunk(uint32 chunkID, int length, void *data,
		                           media_encode_info *info);
		status_t	Write00WBChunk(uint32 chunkID, int length, void *data,
		                           media_encode_info	*info);

		bool	SetStreamCount(int32 streamCount);
		bool	SetCopyright(const char *data);
		
		//	Accessors
		inline AVIHeader		*GetAVIHeader(){ return &m_AVIHeader; }
		inline AVIStreamHeader	*GetStreamOneHeader(){ return &m_StreamHeaderOne; }
		inline AVIStreamHeader	*GetStreamTwoHeader(){ return &m_StreamHeaderTwo; }
		inline AVIVIDSHeader	*GetVIDSHeader(){ return &m_VIDSHeader; }
		inline AVIAUDSHeader	*GetAUDSHeader(){ return &m_AUDSHeader; }
		inline uint32			GetTotalFrames(){ return m_TotalFrames; }
		
	private:
		//	Member Functions
		void	Init();
				
		//	Chunk handling
		bool 	WriteRIFFChunk(uint32 size);
		bool	WriteLISTChunk(uint32 size);
		bool	WriteavihChunk();
		bool	WriteAVIHeader();
		bool	WriteStreamHeader(AVIStreamHeader *header);
		bool	WriteStreamFormat(AVIStreamHeader *header);
		bool 	WritevidsChunk();
		bool 	WriteaudsChunk();
		bool 	Writeidx1Chunk();
		void	AddIndex(uint32 chunkID, off_t pos, size_t length,int32 flags);
								
		//	Member Variables
		//BFile		*m_File;
		FileWriter	*m_File;
		//off_t		datapos;
		bool		raw_data;
		
		uint32		m_AVISize;
		int32		m_StreamCount;
		int32		m_MoviChunkOffset;
		
		bool		m_InitComplete;
		bool		m_UseIndexFlag;
		
		// copyright info
		char		*m_Copyright;

		//	Video Variables
		AVIVIDSHeader	m_VIDSHeader;
		uint32			m_IndexCount;
		uint32			m_TotalFrames;
		
		//	Audio Variables
		AVIAUDSHeader	m_AUDSHeader;		
		uint32			m_AudioDivisor;     // used when writing audio chunks
		uint32			m_TotalAudioTime;
		
		//	Chunk headers
		AVIHeader		m_AVIHeader;		
		off_t			m_streamOnePos;
		AVIStreamHeader	m_StreamHeaderOne;
		off_t			m_streamTwoPos;
		AVIStreamHeader	m_StreamHeaderTwo;

		// Chunk index
		AVIRawIndex 	*m_Idx1Info;
		int64			m_Idx1Count;
		int64			m_MaxIdx1Count;
};

#endif
