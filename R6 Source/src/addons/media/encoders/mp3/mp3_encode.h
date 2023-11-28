
#if !defined(mp3_encode_h)
#define mp3_encode_h

namespace BPrivate {

struct mp3_config
{
	// may be 48000, 44100, or 32000
	int32		framerate;
	int32		bitrate;
	enum mode_t
	{
		MODE_STEREO       = 0,
		MODE_DUAL_CHANNEL = 2,
		MODE_MONO         = 3
	}			mode;
	enum emphasis_t
	{
		EMPH_NONE,
		EMPH_50_150_USEC,
		EMPH_CCITT_J_17
	} 			emphasis;
	bool		priv;
	bool		crc;
	bool		copyright;
	bool		original;
	bool		psycho;
};

}; // BPrivate


/*	create a context cookie */
extern "C" int mp3_init(BPrivate::mp3_config * config, void ** cookie);
/*	encode successive blocks; NULL src for flush; flush (only) before calling done()	*/
extern "C" int mp3_encode(void * cookie, const void * src, int size, void * dest);
/*	dispose cookie	*/
extern "C" int mp3_done(void * cookie);

#endif	//	mp3_encode_h

