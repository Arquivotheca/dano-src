#ifndef VideoDecoder_h
#define Videodecoder_h

#include <Content.h>
#include <View.h>
#include <Locker.h>

//#include "VideoPlayer.h"

class BBitmap;
class BMediaFile;
class BMediaTrack;
class BSoundPlayer;



//class VideoDecoder;
//class AudioDecoder;
class VideoContentInstance;


class VideoDecoder
{
public:
	VideoDecoder(VideoContentInstance * content,
				BBitmap * bitmap,
				BView * view,
				bool useoverlay,
				const media_raw_video_format & fmt,
				BMediaTrack * track);
	~VideoDecoder();

	void			Reset();	//	only call after VideoDone() has been called
	void 			DecodeFrame();
	void			RequestFrame(bigtime_t time,int32 aframe,float arate);
	void			SetPlay(bool play);
private:
	VideoContentInstance * m_daddy;
	BMediaTrack *	m_videoTrack;
	BBitmap *		m_bitmap;
	thread_id		m_decodeThread;
	bigtime_t 		m_atime;
	int32			m_aframe;
	float			m_arate;
	int32			m_vframe;
	float			m_vrate;
	int32 			m_quit;
	BLocker			m_lock;
	bool 			m_play;
	bool			m_go;
	BView * 		m_oview;
	bool			m_useoverlay;
	
	static status_t	DecodeVideoEnter(void *);
	void			DecodeVideo();
	
};

class AudioDecoder
{
public:
	AudioDecoder(VideoContentInstance * content,const media_raw_audio_format & fmt, BMediaTrack * track);
	~AudioDecoder();

	void Reset();	//	only call after AudioDone() has been called
	void SetPlay(bool playing);
	void AddVideoDecoder(VideoDecoder * videodecoder);
	void RemoveVideoDecoder();
private:
	VideoContentInstance * m_daddy;
	BMediaTrack *	m_audioTrack;
	void *			m_fifo;
	sem_id			m_fifoFront;
	int32			m_fifoBack;
	size_t			m_fifoChunk;
	size_t			m_fifoSize;
	size_t			m_fifoPos;
	size_t			m_frameSize;
	size_t			m_playTotal;
	float			m_frameRate;
	BSoundPlayer *	m_soundPlayer;
	thread_id		m_decodeThread;
	bool			m_playing;
	BLocker			m_lock;

	VideoDecoder * 	m_videodecoder;
	
	static status_t DecodeAudioEnter(void * arg);
	void			DecodeAudio();

	static void PlayFunc(void * cookie, void * buf, size_t size, const media_raw_audio_format & fmt);
	void Play(void * buf, size_t size);

};

#endif
