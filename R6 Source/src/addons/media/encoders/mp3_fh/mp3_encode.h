
#if !defined(mp3_encode_h)
#define mp3_encode_h

#include "library/mp3encode.h"

namespace BPrivate {

struct mp3_config
{
	// may be 48000, 44100, or 32000
	int32		framerate;
	int32		bitrate;
	int32		numchannels;
	mp3EncQualityMode	quality;

	bool		crc;
	bool		copyright;
	bool		original;
};

}; // BPrivate


/*	create a context cookie */
extern "C" int mp3_init(BPrivate::mp3_config * config, void ** cookie);
/*	encode successive blocks; NULL src for flush; flush (only) before calling done()	*/
extern "C" int mp3_encode(void * cookie, const void * src, int size, void * dest);
/*	dispose cookie	*/
extern "C" int mp3_done(void * cookie);

#endif	//	mp3_encode_h

