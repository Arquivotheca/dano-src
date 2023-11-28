#ifndef C_MPEG2_TRACK_H

#define C_MPEG2_TRACK_H

#include <SupportDefs.h>
#include <MediaDefs.h>

class BMallocIO;
class CBitStream;

class CMPEG2Track
{
	CMPEG2Track (const CMPEG2Track &);
	CMPEG2Track &operator= (const CMPEG2Track &);
	
	uint8 fStreamID;
	uint8 fSubStreamID;
	media_format fFormat;

	BMallocIO *fStreamData;
	
	// video
	
	struct mpeg_video_data_t
	{
		bool is_mpeg2;

		uint16 horizontal_size;
		uint16 vertical_size;
		float frame_rate;
		uint32 bit_rate;
		bool progressive;
				
	} fVideoData;
			
	bool IdentifyMPEGVideo();
	float GetFrameRate(CBitStream &bs);
	void MakeVideoFormat();

	bool IdentifyAC3Audio();
	
	public:
		CMPEG2Track (uint8 stream_id, media_type type);
		virtual ~CMPEG2Track();
		
//		CMPEG2Track (uint8 stream_id, const media_encoded_video_format *mf, bool is_mpeg2);

		void SetSubStreamID (uint8 substream_id);
				
		uint8 StreamID() const;
		void GetFormat (media_format *format);
		
		void AddData (const void *data, size_t size);
		status_t Sniff();
		
		media_type MediaType() const;
		uint8 SubStreamID() const;
};

#endif
