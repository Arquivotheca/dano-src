#include "MPEG2Track.h"
#include "PositionIOBitStream.h"

#include <MediaFormats.h>
#include <DataIO.h>
#include <Debug.h>

#define C_SEQUENCE_HEADER_CODE 0x1b3
#define C_EXTENSION_START_CODE 0x1b5

#define C_EXTENSION_STARTCODE_SEQUENCE	1

#define AC3_START_CODE	0x0b77

enum
{
	C_AC3_AUDIO = 'AC3A'		// temporary define
};

struct framesize_s
{
	unsigned short bit_rate;
	unsigned short frm_size[3];
};

const struct framesize_s framesize[] = 
{
      { 32  ,{64   ,69   ,96   } },
      { 32  ,{64   ,70   ,96   } },
      { 40  ,{80   ,87   ,120  } },
      { 40  ,{80   ,88   ,120  } },
      { 48  ,{96   ,104  ,144  } },
      { 48  ,{96   ,105  ,144  } },
      { 56  ,{112  ,121  ,168  } },
      { 56  ,{112  ,122  ,168  } },
      { 64  ,{128  ,139  ,192  } },
      { 64  ,{128  ,140  ,192  } },
      { 80  ,{160  ,174  ,240  } },
      { 80  ,{160  ,175  ,240  } },
      { 96  ,{192  ,208  ,288  } },
      { 96  ,{192  ,209  ,288  } },
      { 112 ,{224  ,243  ,336  } },
      { 112 ,{224  ,244  ,336  } },
      { 128 ,{256  ,278  ,384  } },
      { 128 ,{256  ,279  ,384  } },
      { 160 ,{320  ,348  ,480  } },
      { 160 ,{320  ,349  ,480  } },
      { 192 ,{384  ,417  ,576  } },
      { 192 ,{384  ,418  ,576  } },
      { 224 ,{448  ,487  ,672  } },
      { 224 ,{448  ,488  ,672  } },
      { 256 ,{512  ,557  ,768  } },
      { 256 ,{512  ,558  ,768  } },
      { 320 ,{640  ,696  ,960  } },
      { 320 ,{640  ,697  ,960  } },
      { 384 ,{768  ,835  ,1152 } },
      { 384 ,{768  ,836  ,1152 } },
      { 448 ,{896  ,975  ,1344 } },
      { 448 ,{896  ,976  ,1344 } },
      { 512 ,{1024 ,1114 ,1536 } },
      { 512 ,{1024 ,1115 ,1536 } },
      { 576 ,{1152 ,1253 ,1728 } },
      { 576 ,{1152 ,1254 ,1728 } },
      { 640 ,{1280 ,1393 ,1920 } },
      { 640 ,{1280 ,1394 ,1920 } }
};

const uint32
channel_count[8] = { 2,1,2,3,3,4,4,5 };

CMPEG2Track::CMPEG2Track (uint8 stream_id, media_type type)
	: fStreamID(stream_id),
	  fSubStreamID(0),
	  fStreamData(NULL)
{
}

CMPEG2Track::~CMPEG2Track()
{
	delete fStreamData;
}

#if 0

CMPEG2Track::CMPEG2Track(uint8 stream_id, const media_encoded_video_format *mf, bool is_mpeg2)
	: fStreamID(stream_id)
{
	fFormat.type=B_MEDIA_ENCODED_VIDEO;
	fFormat.u.encoded_video=media_encoded_video_format::wildcard;

	media_format_description desc;
	desc.family=B_MPEG_FORMAT_FAMILY;
	desc.u.mpeg.id=is_mpeg2 ? B_MPEG_1_VIDEO+1 : B_MPEG_1_VIDEO;

	BMediaFormats mfo;
	
	if (mfo.GetFormatFor(desc,&fFormat)<B_OK)
	{
		if (mfo.MakeFormatFor(&desc,1,&fFormat)<B_OK)
			TRESPASS();
	}
		
	media_encoded_video_format::video_encoding saved_encoding
		=fFormat.u.encoded_video.encoding;
		
	fFormat.u.encoded_video=*mf;
	fFormat.u.encoded_video.encoding=saved_encoding;
}

#endif

uint8 
CMPEG2Track::StreamID() const
{
	return fStreamID;
}

void 
CMPEG2Track::GetFormat(media_format *format)
{
	*format=fFormat;
}

void 
CMPEG2Track::AddData(const void *data, size_t size)
{
	if (!fStreamData)
		fStreamData=new BMallocIO;
		
	fStreamData->Write(data,size);
}

status_t 
CMPEG2Track::Sniff()
{
	bool identified=false;
	
	if (fStreamID==0xe0)
		identified=IdentifyMPEGVideo() && fVideoData.is_mpeg2; // reject mpeg1 for now
	else if (fStreamID==0xbd)
		identified=IdentifyAC3Audio();
	
	delete fStreamData;
	fStreamData=NULL;
	
	return identified ? B_OK : B_ERROR;
}

bool 
CMPEG2Track::IdentifyAC3Audio()
{
	uint8 *temp=(uint8 *)fStreamData->Buffer();

	bool identified=false;
	
	fStreamData->Seek(0,SEEK_SET);
	
	CPositionIOBitStream bs(fStreamData,false);

	try
	{
		uint8 substream_id=bs.GetBits(8);
		
		if ((substream_id & ~7)!=0x80)
			return false; // this is not AC-3
			
		bs.SeekByteBoundary();
		
		while (bs.PeekBits(16)!=AC3_START_CODE)
			bs.SkipBits(8);
			
		bs.SkipBits(32);	// Skip start code and crc
		
		uint8 sampling_freq_code=bs.GetBits(2);
		ASSERT(sampling_freq_code<3);
		
		const float sampling_freq[4]={ 48000.0f,44100.0f,32000.0f, 0.0f };
		
		uint8 framesize_code=bs.GetBits(6);
		
		bs.SkipBits(5);
		bs.SkipBits(3);
		
		uint8 audio_coding_mode=bs.GetBits(3);
		
		fFormat.type=B_MEDIA_ENCODED_AUDIO;
		fFormat.u.encoded_audio=media_encoded_audio_format::wildcard;
	
		media_format_description desc;
		desc.family=B_BEOS_FORMAT_FAMILY;
		desc.u.beos.format=C_AC3_AUDIO;
	
		BMediaFormats mfo;
		
		if (mfo.GetFormatFor(desc,&fFormat)<B_OK)
		{
			if (mfo.MakeFormatFor(&desc,1,&fFormat)<B_OK)
				TRESPASS();
		}
		
		fFormat.u.encoded_audio.output.frame_rate=sampling_freq[sampling_freq_code];
		fFormat.u.encoded_audio.output.channel_count=channel_count[audio_coding_mode];
		fFormat.u.encoded_audio.bit_rate=framesize[framesize_code].bit_rate;
		fFormat.u.encoded_audio.frame_size=2*framesize[framesize_code].frm_size[sampling_freq_code];

		identified=true;		
	}
	catch (CBitStream::eof_exception &e)
	{
	}
	
	return identified;
}

bool
CMPEG2Track::IdentifyMPEGVideo()
{
	bool identified=false;

	fStreamData->Seek(0,SEEK_SET);
	
	CPositionIOBitStream bs(fStreamData,false);

	try
	{
		while (!identified)
		{
			bs.SeekByteBoundary();
			
			while (bs.PeekBits(24)!=0x000001)
				bs.SkipBits(8);
			
			uint32 code=bs.GetBits(32);
			
			switch (code)
			{
				case C_SEQUENCE_HEADER_CODE:
				{
					fVideoData.is_mpeg2=false;
					
					fVideoData.horizontal_size=bs.GetBits(12);
					fVideoData.vertical_size=bs.GetBits(12);
				
					bs.SkipBits(4);
					
					fVideoData.frame_rate=GetFrameRate(bs);
						
					fVideoData.bit_rate=bs.GetBits(18);
				
					DEBUG_ONLY(uint8 marker=)bs.GetBits(1);
					ASSERT(marker);
					
					bs.SkipBits(11);
					
					if (bs.GetBits(1))
						bs.SkipBytes(64);

					if (bs.GetBits(1))
						bs.SkipBytes(64);

					fVideoData.progressive=true;

					bs.SeekByteBoundary();
					
					while (bs.PeekBits(24)!=0x000001)
						bs.SkipBits(8);
						
					if (bs.GetBits(32)==C_EXTENSION_START_CODE)
					{						
						uint8 extension_code=bs.GetBits(4);
						
						if (extension_code==C_EXTENSION_STARTCODE_SEQUENCE)
						{
							fVideoData.is_mpeg2=true;
							
							bs.SkipBits(8);
						
							fVideoData.progressive=bs.GetBits(1);
						
							bs.SkipBits(2);	
							
							uint8 hor_size_extension=bs.GetBits(2);
							
							fVideoData.horizontal_size|=uint32(hor_size_extension)<<14;
						
							uint8 ver_size_extension=bs.GetBits(2);
						
							fVideoData.vertical_size|=uint32(ver_size_extension)<<14;
						
							uint16 bitrate_extension=bs.GetBits(12);
							
							fVideoData.bit_rate|=uint32(bitrate_extension)<<18;
							
							DEBUG_ONLY(uint8 marker=)bs.GetBits(1);
							ASSERT(marker);
							
							bs.SkipBits(9);
							
							uint8 frame_rate_extension_n=bs.GetBits(2);
							uint8 frame_rate_extension_d=bs.GetBits(5);
							
							fVideoData.frame_rate*=float(frame_rate_extension_n+1)/float(frame_rate_extension_d+1);
						}
					}
					
					MakeVideoFormat();
					identified=true;
				}
				break;				
			}	
		}
	}
	catch (CBitStream::eof_exception &e)
	{
	}
	
	return identified;
}

float 
CMPEG2Track::GetFrameRate (CBitStream &bs)
{
	const float kFrameRateTable[] = { 24000.0f/1001.0f,24.0f,25.0f,30000.0f/1001.0f,
										30.0f,50.0f,60000.0f/1001.0f,60.0f };

	uint8 code=bs.GetBits(4);
	
	ASSERT(code>=1 && code<=8);
	
	return kFrameRateTable[code-1];
}

void
CMPEG2Track::MakeVideoFormat()
{
	fFormat.type=B_MEDIA_ENCODED_VIDEO;
	fFormat.u.encoded_video=media_encoded_video_format::wildcard;

	media_format_description desc;
	desc.family=B_MPEG_FORMAT_FAMILY;
	desc.u.mpeg.id=fVideoData.is_mpeg2 ? B_MPEG_1_VIDEO+1 : B_MPEG_1_VIDEO;

	BMediaFormats mfo;
	
	if (mfo.GetFormatFor(desc,&fFormat)<B_OK)
	{
		if (mfo.MakeFormatFor(&desc,1,&fFormat)<B_OK)
			TRESPASS();
	}
		
	fFormat.u.encoded_video.max_bit_rate=fVideoData.bit_rate*400.0f;

	media_raw_video_format *rvf=&fFormat.u.encoded_video.output;
	
	rvf->field_rate=fVideoData.frame_rate;
	rvf->interlace=fVideoData.progressive ? 1 : 2;
	rvf->last_active=fVideoData.vertical_size-1;
	rvf->display.line_width=fVideoData.horizontal_size;
	rvf->display.line_count=fVideoData.vertical_size;
}

media_type 
CMPEG2Track::MediaType() const
{
	return fFormat.type;
}

uint8 
CMPEG2Track::SubStreamID() const
{
	return fSubStreamID;
}

void 
CMPEG2Track::SetSubStreamID(uint8 substream_id)
{
	fSubStreamID=substream_id;
}

