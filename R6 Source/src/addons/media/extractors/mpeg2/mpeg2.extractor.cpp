#include "mpeg2.extractor.h"

#include "PositionIOBitStream.h"
#include "MemoryBitStream.h"

#include <DataIO.h>
#include <File.h>

#include <Debug.h>
#include <NetDebug.h>

#define C_PACKET_START_CODE_PREFIX 1

#define C_MPEG_PROGRAM_END_CODE 0x1b9
#define C_PACK_START_CODE 0x1ba
#define C_SYSTEM_HEADER_START_CODE 0x1bb
#define C_SEQUENCE_HEADER_CODE 0x1b3
#define C_EXTENSION_START_CODE 0x1b5
#define C_GOP_START_CODE 0x1b8
#define C_PICTURE_START_CODE 0x100

#define C_STUFFING_BYTE 0xff
#define C_PADDING_BYTE 0xff

#define C_SYNC_BYTE 0x47

#define C_EXTENSION_STARTCODE_SEQUENCE 					1
#define C_EXTENSION_STARTCODE_SEQUENCE_DISPLAY			2
#define C_EXTENSION_STARTCODE_QUANT_MATRIX				3
#define C_EXTENSION_STARTCODE_COPYRIGHT					4
#define C_EXTENSION_STARTCODE_SEQUENCE_SCALABLE			5
#define C_EXTENSION_STARTCODE_PICTURE_DISPLAY			7
#define C_EXTENSION_STARTCODE_PICTURE_CODING			8
#define C_EXTENSION_STARTCODE_PICTURE_SPATIAL_SCALABLE	9
#define C_EXTENSION_STARTCODE_PICTURE_TEMPORAL_SCALABLE	10

#define C_DATA_PARTITIONING 	0
#define C_SPATIAL_SCALABILITY 	1
#define C_SNR_SCALABILITY 		2
#define C_TEMPORAL_SCALABILITY 	3

#define C_I_PICTURE 1
#define C_P_PICTURE 2
#define C_B_PICTURE 3

// stream-id types
#define C_PROGRAM_STREAM_MAP_STREAM_ID 			0xbc		/* 10111100 */
#define C_PRIVATE_STREAM_1_STREAM_ID			0xbd		/* 10111101 */
#define C_PADDING_STREAM_STREAM_ID				0xbe		/* 10111110 */
#define C_PRIVATE_STREAM_2_STREAM_ID			0xbf		/* 10111111 */
#define C_MPEG_AUDIO_STREAM_MASK				0xc0		/* 110xxxxx, mask ~0x1f */
#define C_MPEG_VIDEO_STREAM_MASK				0xe0		/* 1110xxxx, mask ~0x0f */
#define C_ECM_STREAM_ID							0xf0		/* 11110000 */
#define C_EMM_STREAM_ID							0xf1		/* 11110001 */
#define C_DSMCC_STREAM_ID						0xf2		/* 11110010 */
#define C_ISOIEC_13522_STREAM_ID				0xf3		/* 11110011 */
#define C_ITU_T_H222_1_A_STREAM_ID				0xf4		/* 11110100 */
#define C_ITU_T_H222_1_B_STREAM_ID				0xf5		/* 11110101 */
#define C_ITU_T_H222_1_C_STREAM_ID				0xf6		/* 11110110 */
#define C_ITU_T_H222_1_D_STREAM_ID				0xf7		/* 11110111 */
#define C_ITU_T_H222_1_E_STREAM_ID				0xf8		/* 11111000 */
#define C_ANCILLARY_STREAM_ID					0xf9		/* 11111001 */
#define C_PROGRAM_STREAM_DIRECTORY_STREAM_ID	0xff		/* 11111111 */

BPrivate::Extractor *
instantiate_extractor(void)
{
	return new CMPEG2Extractor;
}

CMPEG2Extractor::CMPEG2Extractor()
{
}

CMPEG2Extractor::~CMPEG2Extractor()
{
}

status_t 
CMPEG2Extractor::SniffPESPacket(CBitStream &bs, bool scan_tracks,
								uint8 *which_stream, uint8 *which_substream,
								size_t *chunk_length)
{
	uint8 stream_id=bs.GetBits(8);

	uint16 length=bs.GetBits(16);
	
	if (stream_id!=C_PROGRAM_STREAM_MAP_STREAM_ID
		&& stream_id!=C_PADDING_STREAM_STREAM_ID
		&& stream_id!=C_PRIVATE_STREAM_2_STREAM_ID
		&& stream_id!=C_ECM_STREAM_ID
		&& stream_id!=C_EMM_STREAM_ID
		&& stream_id!=C_PROGRAM_STREAM_DIRECTORY_STREAM_ID
		&& stream_id!=C_DSMCC_STREAM_ID
		&& stream_id!=C_ITU_T_H222_1_E_STREAM_ID)
	{
		CMPEG2Track *track=NULL;
		
		if (scan_tracks)
		{
			media_type type;
			
			if ((stream_id & ~0x1f)==C_MPEG_AUDIO_STREAM_MASK)
				type=B_MEDIA_ENCODED_AUDIO;
			else if ((stream_id & ~0x0f)==C_MPEG_VIDEO_STREAM_MASK)
				type=B_MEDIA_ENCODED_VIDEO;
			else
				type=B_MEDIA_NO_TYPE;
			
			if (type!=B_MEDIA_NO_TYPE)
			{
				bool already_there=false;
				for (int32 i=fTracks.CountItems()-1;!already_there && i>=0;--i)
				{
					if (fTracks[i]->StreamID()==stream_id)
					{
						already_there=true;
						track=fTracks[i];
					}
				}
				
				if (!already_there)
					fTracks.AddItem(track=new CMPEG2Track(stream_id,type));
			}
		}
		
		uint8 signature=bs.PeekBits(2);

		uint16 data_length;
		
		if (signature==2)	// MPEG-2 Elementary Stream
		{
			bs.SkipBits(2);

			/*uint8 scrambling_control=*/bs.GetBits(2);
			
			bs.SkipBits(12);
			
			uint8 PES_header_data_length=bs.GetBits(8);
			
			bs.SkipBytes(PES_header_data_length);
			
			data_length=length-PES_header_data_length-3;
		}
		else	// MPEG-1 System Stream
		{
			uint16 header_bits=0;
			
			while (bs.PeekBits(8)==C_STUFFING_BYTE)
			{
				bs.SkipBits(8);
				header_bits+=8;
			}
				
			if ((bs.PeekBits(8) & 0x40)==0x40)
			{
				bs.SkipBits(16);
				header_bits+=16;
			}
			
			uint8 pts_dts_flags=bs.PeekBits(8);
			
			if (pts_dts_flags>=0x30)
			{
				bs.SkipBits(80);
				header_bits+=80;
			}
			else if (pts_dts_flags>=0x20)
			{
				bs.SkipBits(40);
				header_bits+=40;
			}
			else if (pts_dts_flags==0x0f)
			{
				bs.SkipBits(8);
				header_bits+=8;
			}
			else
				TRESPASS();
			
			ASSERT((header_bits%8)==0);
			
			data_length=length-(header_bits/8);
		}

		if (!scan_tracks)
		{
			*which_stream=stream_id;
			*which_substream=0;
			*chunk_length=data_length;
			
			if (stream_id==C_PRIVATE_STREAM_1_STREAM_ID)
			{
				*which_substream=bs.PeekBits(8);
				
				bs.SkipBits(32);				// the first 4 bytes are actually
				*chunk_length=data_length-4;	// some kind of junk
			}
			
			return B_OK;
		}
		else
		{
			if (stream_id==C_PRIVATE_STREAM_1_STREAM_ID)
			{
				uint8 substream_id=bs.PeekBits(8);
				
				bool already_there=false;
				for (int32 i=fTracks.CountItems()-1;!already_there && i>=0;--i)
				{
					if (fTracks[i]->StreamID()==stream_id && fTracks[i]->SubStreamID()==substream_id)
					{
						already_there=true;
						track=fTracks[i];
					}
				}
				
				if (!already_there)
				{
					fTracks.AddItem(track=new CMPEG2Track(stream_id,B_MEDIA_ENCODED_AUDIO));
					track->SetSubStreamID(substream_id);
				}
			}

			if (track)
			{
				if (data_length>fBufferSize)
				{
					fBufferSize=(data_length+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1);
					
					fBuffer=realloc(fBuffer,fBufferSize);
				}

				if (stream_id==C_PRIVATE_STREAM_1_STREAM_ID)
				{
					uint32 dummy=bs.PeekBits(32);
					
					PRINT(("%02x %02x %02x %02x\n",dummy>>24,(dummy>>16)&0xff,(dummy>>8)&0xff,dummy&0xff));					
				}
				
				bs.GetBytes(fBuffer,data_length);				
				track->AddData(fBuffer,data_length);
			}
			else
				bs.SkipBytes(data_length);
		}
	}
	else
	{
		if (!scan_tracks)
		{
			*which_stream=0;
			*chunk_length=length;
		}
		else
			bs.SkipBytes(length);
	}
	
	return B_OK;
}

status_t
CMPEG2Extractor::SniffSystemHeader(CBitStream &bs)
{
	uint32 code=bs.GetBits(32);
	ASSERT(code==C_SYSTEM_HEADER_START_CODE);
	
	uint16 header_length=bs.GetBits(16);

	uint8 marker=bs.GetBits(1);
	ASSERT(marker);

	/*uint32 rate_bound=*/bs.GetBits(22);

	marker=bs.GetBits(1);
	ASSERT(marker);

	/*uint8 audio_bound=*/bs.GetBits(6);
	
	/*uint8 fixed_flag=*/bs.GetBits(1);
	/*uint8 CSPS_flag=*/bs.GetBits(1);
	/*uint8 system_audio_lock_flag=*/bs.GetBits(1);
	/*uint8 system_video_lock_flag=*/bs.GetBits(1);
	
	marker=bs.GetBits(1);
	ASSERT(marker);
	
	/*uint8 video_bound=*/bs.GetBits(5);
	/*uint8 packet_rate_restriction_flag=*/bs.GetBits(1);
	/*uint8 reserved_byte=*/bs.GetBits(7);

	uint32 bits_read=48;
	
	while (bs.PeekBits(1))
	{
		/*uint8 stream_id=*/bs.GetBits(8);

		uint8 marker2=bs.GetBits(2);
		ASSERT(marker2==3);

		/*uint8 P_STD_buffer_bound_scale=*/bs.GetBits(1);
		/*uint16 P_STD_buffer_size_bound=*/bs.GetBits(13);

		bits_read+=24;
	}
	
	bs.SkipBits(header_length*8-bits_read);
	
	return B_OK;
}

status_t
CMPEG2Extractor::SniffPackHeader(CBitStream &bs)
{
//	uint32 code=bs.GetBits(32);
//	ASSERT(code==C_PACK_START_CODE);
	
	uint8 dummy1=bs.GetBits(2);
	ASSERT(dummy1==1);

	uint64 sys_clock_ref_base=0;
	uint8 dummy2=bs.GetBits(3);
	sys_clock_ref_base|=uint64(dummy2)<<30;

	uint8 marker=bs.GetBits(1);
	ASSERT(marker);
	
	uint16 dummy3=bs.GetBits(15);
	sys_clock_ref_base|=uint64(dummy3)<<15;
			
	marker=bs.GetBits(1);
	ASSERT(marker);

	uint16 dummy4=bs.GetBits(15);
	sys_clock_ref_base|=dummy4;

	marker=bs.GetBits(1);
	ASSERT(marker);

	/*uint16 sys_clock_ref_extension=*/bs.GetBits(9);

	marker=bs.GetBits(1);
	ASSERT(marker);

	/*uint32 program_mux_rate=*/bs.GetBits(22);

	uint8 marker2=bs.GetBits(2);
	ASSERT(marker2==3);
	
	bs.SkipBits(5);

	uint8 pack_stuffing_length=bs.GetBits(3);
	for (uint8 i=0;i<pack_stuffing_length;++i)
	{
		uint8 stuffing_byte=bs.GetBits(8);
		ASSERT(stuffing_byte==C_STUFFING_BYTE);
	}

	return B_OK;
}

status_t 
CMPEG2Extractor::Sniff (int32 *out_streamNum, int32 *out_chunkSize)
{
	*out_streamNum=0;
	*out_chunkSize=0;

	status_t result=Sniff();
	
	if (result>=B_OK)
	{
		*out_streamNum=fTracks.CountItems();
		*out_chunkSize=fChunkSize;

		if ((*out_streamNum)==0)
			result=B_ERROR;
 	}

	return result;
}

status_t 
CMPEG2Extractor::GetFileFormatInfo (media_file_format *mfi)
{
	mfi->capabilities=media_file_format::B_READABLE
						| media_file_format::B_IMPERFECTLY_SEEKABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO;

	mfi->family=B_MPEG_FORMAT_FAMILY;
	mfi->version=200;
	
	memset(mfi->_reserved_,0,sizeof(mfi->_reserved_));
	
	strcpy(mfi->mime_type,"video/mpeg");
	strcpy(mfi->pretty_name,"MPEG Stream");
	strcpy(mfi->short_name,"mpg");
	
	memset(mfi->reserved,0,sizeof(mfi->reserved));

	return B_OK;
}

status_t 
CMPEG2Extractor::TrackInfo (int32 in_stream, media_format *out_format,
							void **out_info, int32 *out_infoSize)
{
	if (in_stream<0 || in_stream>=fTracks.CountItems())
		return B_BAD_INDEX;

	fTracks[in_stream]->GetFormat(out_format);

	*out_info=NULL;
	*out_infoSize=0;
			
	return B_OK;
}

status_t 
CMPEG2Extractor::CountFrames (int32 in_stream, int64 *out_frames)
{
	TRESPASS();
	
	return B_ERROR;
}

status_t 
CMPEG2Extractor::GetDuration (int32 in_stream, bigtime_t *out_duration)
{
	*out_duration=100000000;
	
	return B_OK;
}

status_t 
CMPEG2Extractor::AllocateCookie (int32 in_stream, void **cookieptr)
{
	*cookieptr=NULL;
	
	return B_OK;
}

status_t 
CMPEG2Extractor::FreeCookie (int32 in_stream, void *cookie)
{
	return B_OK;
}

status_t
CMPEG2Extractor::GetStreamData (CMemoryBitStream &bs,
								uint8 stream_id,
								uint8 substream_id,
								size_t *chunk_length)
{
	bs.SeekByteBoundary();
	while (bs.HasData() && bs.PeekBits(24)!=C_PACKET_START_CODE_PREFIX)
	{
		PRINT(("missing start code prefix 0x%08lx\n",bs.PeekBits(24)));
		bs.SkipBits(8);
	}

	if (!bs.HasData())
		return B_IO_ERROR;
				
	uint32 code=bs.PeekBits(32);

	switch (code)
	{
		case C_MPEG_PROGRAM_END_CODE:
			return B_IO_ERROR;
			
		case C_PACK_START_CODE:
		{
			bs.SkipBits(32);

			if (bs.PeekBits(2)==1) // MPEG-2 Elementary Stream
				SniffPackHeader(bs);
			else if (bs.PeekBits(4)==2) // MPEG-1 System Stream
				bs.SkipBits(64);
			else
				TRESPASS();				
			
			break;
		}
			
		case C_SYSTEM_HEADER_START_CODE:
			SniffSystemHeader(bs);
			break;

		default:
		{
			bs.SkipBits(24);

			uint8 which_stream;
			uint8 which_substream;
			SniffPESPacket(bs,false,&which_stream,&which_substream,chunk_length);
			
			if (which_stream==stream_id && which_substream==substream_id)
				return B_OK;
			else
				bs.SkipBytes(*chunk_length);					
		}
		break;
	}

	return B_ERROR;
}

status_t 
CMPEG2Extractor::SplitNext (int32 in_stream, void *cookie, off_t *inout_filepos,
							char *in_packetPointer, int32 *inout_packetLength,
							char **out_bufferStart, int32 *out_bufferLength,
							media_header *out_mh)
{		
	if (in_stream<0 || in_stream>=fTracks.CountItems())
		return B_BAD_INDEX;

	CMemoryBitStream bs(in_packetPointer,*inout_packetLength);

	try
	{
		size_t chunk_length;
		status_t result=GetStreamData(bs,fTracks[in_stream]->StreamID(),
											fTracks[in_stream]->SubStreamID(),
											&chunk_length);
		
		if (result>=B_OK)	// found a PES-chunk of stream
		{
			memset(out_mh,0,sizeof(*out_mh));
			out_mh->type=fTracks[in_stream]->MediaType();
			out_mh->orig_size=chunk_length;
			out_mh->file_pos=*inout_filepos+bs.Position();

			*inout_filepos+=bs.Position()+chunk_length;
			*inout_packetLength=chunk_length;
			
			*out_bufferStart=in_packetPointer+bs.Position();
			*out_bufferLength=chunk_length;
							
			return B_OK;
		}
		else if (result==B_IO_ERROR)	// end of stream
		{
			*inout_packetLength=0;
	
			*out_bufferStart=NULL;		
			*out_bufferLength=0;
			
			return B_ERROR;
		}
		else	// found nothing worth of returning, just skip it
		{
			*inout_filepos+=bs.Position();
			*inout_packetLength=0;
				
			*out_bufferStart=NULL;		
			*out_bufferLength=0;
		
			return B_OK;
		}
	}
	catch (CBitStream::eof_exception &e)
	{
		// not enough data to return anything sensible (i.e. no complete packet)
		
		*inout_packetLength=fChunkSize;

		*out_bufferStart=NULL;		
		*out_bufferLength=0;
	
		return B_OK;
	}
}

status_t 
CMPEG2Extractor::Seek (int32 in_stream, void *cookie, int32 in_towhat, int32 flags,
						bigtime_t *inout_time, int64 *inout_frame, off_t *inout_filePos,
						char *in_packetPointer, int32 *inout_packetLength, bool *out_done)
{
	return B_ERROR;
}

status_t
CMPEG2Extractor::Sniff()
{
	fBuffer=NULL;
	fBufferSize=0;
	
	BPositionIO *seekable_source=dynamic_cast<BPositionIO *>(Source());
	
	if (!seekable_source)
		return B_ERROR;
		
	CBitStream *bs=new CPositionIOBitStream(seekable_source);
	
	status_t result=ScanProgramStream(*bs);
	
	delete bs;
	
	fChunkSize=256*1024;	//HACK

	free(fBuffer);
	
	return result;
}

status_t
CMPEG2Extractor::ScanProgramStream (CBitStream &bs)
{
	if (bs.PeekBits(32)!=C_PACK_START_CODE)
		return B_ERROR;
		
	int32 sniffed_PES_packets=0;
	
	bool sniffing_done=false;
	while (!sniffing_done)
	{
		bs.SeekByteBoundary();
		while (bs.PeekBits(24)!=C_PACKET_START_CODE_PREFIX)
		{
			PRINT(("missing start code prefix\n"));
			bs.SkipBits(8);
		}
		
		uint32 code=bs.PeekBits(32);

		switch (code)
		{
			case C_MPEG_PROGRAM_END_CODE:
			{
				bs.SkipBits(32);
				
				sniffing_done=true;
				break;
			}

			case C_PACK_START_CODE:
			{
				bs.SkipBits(32);

				if (bs.PeekBits(2)==1) // MPEG-2 Elementary Stream
					SniffPackHeader(bs);
				else if (bs.PeekBits(4)==2) // MPEG-1 System Stream
					bs.SkipBits(64);
				else
					TRESPASS();				
				
				break;
			}
				
			case C_SYSTEM_HEADER_START_CODE:
				SniffSystemHeader(bs);
				break;
			
			default:
			{
				bs.SkipBits(24);

				SniffPESPacket(bs,true);
				
				if (++sniffed_PES_packets==100)
					sniffing_done=true;
			}
			break;
		}
	}

	for (int32 i=fTracks.CountItems()-1;i>=0;--i)
	{
		if (fTracks[i]->Sniff()<B_OK)
			delete fTracks.RemoveItem(i);
	}
		
	return B_OK;
}
