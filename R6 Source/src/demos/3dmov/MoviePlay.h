// ===========================================================================
//	MoviePlay.h
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __MOVIEPLAY__
#define __MOVIEPLAY__

#include "STORE.H"
#include "Cinepak.h"

//====================================================================

typedef struct {
	long	flags;
	long	creationTime;
	long	modificationTime;
	long	timeScale;
	long	duration;
	short	language;
	short	quality;
} MediaHeader;

//====================================================================
//	the data handler looks after data delivery
//	Normally used by more than one piece of media

class DataSource {
public:
	virtual	long	GetDataLength();
	virtual	void	*GetData(long offset, long count);
	virtual	void	ReleaseData(void *data);
protected:
};

//====================================================================
//	Sample description

typedef struct {
	long	descSize;
	long	dataFormat;
	long	resvd1;
	short	resvd2;
	short	dataRefIndex;
} SampleDesc;

//	Sound description

typedef struct {
	long	descSize;
	long	dataFormat;
	long	resvd1;
	short	resvd2;
	short	dataRefIndex;
	short	version;		// which version is this data
	short	revlevel;		// what version of that codec did this
	long	vendor;			// whose  codec compressed this data
	short	numChannels;	// number of channels of sound
	short	sampleSize;		// number of bits per sample
	short	compressionID;	// sound compression used, 0 if none
	short	packetSize;		// packet size for compression, 0 if no compression
	long	sampleRate;		// sample rate sound is captured at
} SoundDesc;

//	Image description

typedef struct {
	long	idSize;			// total size of ImageDesc
	long	cType;			// Compression type
	long	resvd1;
	short	resvd2;
	short	dataRefIndex;
	short	version;
	short	revisionLevel;
	long	vendor;
	long	temporalQuality;
	long	spatialQuality;
	short	width;			// how many pixels wide is this data
	short	height;			// how many pixels high is this data
	long	hRes;			// resolution
	long	vRes;	
	long	dataSize;		// if known, the size of data for this image descriptor
	short	frameCount;		// number of frames this description applies to
	char	name[32];		// pascal string
	short	depth;			// what depth is this data (1-32) or (33-40 grayscale)
	short	clutID;			// clut id or if 0 clut follows  or -1 if no clut
} ImageDesc;

//====================================================================

class VideoCodec {
public:
	virtual			~VideoCodec();
	
	virtual	Boolean	PreDecompress(ImageDesc *desc, long *width, long *height);	// Codec may pad width and height
	virtual	Boolean	Decompress(void *data, ImageDesc *desc, Byte *dst, long dstRowBytes, long depth);
protected:
};

class CinepakCodec : public VideoCodec {
public:
					CinepakCodec();
	virtual			~CinepakCodec();
	
	virtual	Boolean	PreDecompress(ImageDesc *desc, long *width, long *height);	// Codec may pad width and height
	virtual	Boolean	Decompress(void *data, ImageDesc *desc, Byte *dst, long dstRowBytes, long depth);
protected:
		long		mCodeBook[2048*4];	// Up to four detailed codebooks
};

class QTPlayer;

//====================================================================
//	MediaHandlers

class MediaHndlr {
public:
					MediaHndlr();
	virtual			~MediaHndlr();
	
	virtual	void	SetDescription(void *desc);
	virtual	void	Display(void *data, long count);
	virtual	Boolean	ProcessInChunks();

	virtual	void	StartPlaying();
	virtual	void	StopPlaying();
	
protected:
	QTPlayer*	mPlayer;
};

class VideoMediaHandler : public MediaHndlr {
public:
					VideoMediaHandler(QTPlayer *player);
	virtual			~VideoMediaHandler();
	
	virtual	void	SetDescription(void *desc);
	virtual	void	Display(void *data, long count);
	
protected:
	ImageDesc*	mImageDesc;
	VideoCodec*	mVideoCodec;
	long		mCType;
	long		mWidth;
	long		mHeight;
	
	Byte*		mBaseAddr;
	long		mRowBytes;
	long		mDepth;
};

class SoundMediaHandler : public MediaHndlr {
public:
					SoundMediaHandler(QTPlayer *player);
	virtual			~SoundMediaHandler();
	
	virtual	void	SetDescription(void *desc);
	virtual	void	Display(void *data, long count);
	virtual	Boolean	ProcessInChunks();

	virtual	void	StartPlaying();
	virtual	void	StopPlaying();
	
protected:
	SoundDesc*		mSoundDesc;
	bool			mKnownFormat;
};


//====================================================================
//	SampleTable manages data about sample size, data location and playback time

class QTSampleTable {
public:
					QTSampleTable(Byte *atom);
	virtual			~QTSampleTable();	
			
			long	TimeToSample(long *time, long *duration);
			long	SampleToChunk(long sample, long *firstSampleInChunk, long *samplesInChunk, long *descID);
			
			long	GetSampleDescriptionCount();
			void*	GetSampleDescription(long index);
			long	GetSampleCount();
			long	GetChunkCount();

			Boolean	ProcessChunks();
			Boolean	GetSample(long *time, long *duration, long *offset, long *size, long *descID);
			Boolean	GetSampleChunk(long *time, long *duration, long *count, long *offset, long *size, long *descID);

protected:
			long*	mSampleDesc;
			long*	mTimeToSamp;
			long*	mSynchSamp;
			long*	mSampleToChunk;
			long*	mSampleSize;
			long*	mChunkOffset;
};

	
//====================================================================
//	Play lists are generated in groups to allow better i/o
//	scheduling and playback performance management
//
//	Playlists contain the decoded sample information from all media in
//	play sequence. A QTSampleInfo contains the track, sample position
//	in the media, duration, synch flags etc.
//
//	The playback code generates chunks of playback lists, then uses the
//	'flattened' sample info to play the samples
//

class QTSampleInfo : public NPObject {
public:
		void		Print();
		
		long		mTime;				// Sample time in its own timescale
		long		mDuration;			// Duration or Duration of Chunk
		long		mCount;				// Number of samples
		bool		mSynch;				// Is this a synch sample
		
		long		mOffset;			// Position in media
		long		mSize;				// Size of data
		
		long		mDescID;			// Media descrptor id
};

//	List of sampleInfo

class QTTrack;
class QTSampleList : public NPObject {
public:
					QTSampleList();
	void			Create(QTTrack* track);
	long			Count();
	QTSampleInfo&	Get(int index);
	
protected:
	CArray<QTSampleInfo,128>	mList;
	long	mCount;
};

//====================================================================

class QTMedia {
public:
					QTMedia(Byte *atom, QTPlayer *player);
	virtual			~QTMedia();	

		long		GetMediaType();
		long		GetTimeScale();
		
//		Get a play list QTSampleInfo entry for this time
//		Time will be aligned to the start of the nearest chunk

		Boolean		GetSampleInfo(long time, QTSampleInfo& play);
		
//		Play the sample now

		Boolean		PlaySample(QTSampleInfo& play);

		void		StartPlaying();
		void		StopPlaying();
		
protected:
		void		InitMediaInfo(Byte *atom);
		
		QTPlayer*		mPlayer;
		MediaHeader		mMediaHeader;
		MediaHndlr*		mHandler;
		QTSampleTable*	mSampleTable;
		long			mLastDescID;
		long			mMediaType;
};

//====================================================================

class QTEditList {
public:
			QTEditList(Byte *atom);
	virtual	~QTEditList();	
			
protected:
};

//====================================================================

typedef struct {
	long	flags;
	long	creationTime;
	long	modificationTime;
	long	trackID;
	long	reserved;
	long	duration;
	long	reserved2[2];
	short	layer;
	short	alternateGroup;
	short	volume;
	short	reserved3;
	long	matrix[9];
	long	width;
	long	height;
} TrackHeader;

//====================================================================

class QTTrack : public CLinkable {
public:
					QTTrack(Byte *atom, QTPlayer *player);
		virtual		~QTTrack();
	
		Boolean		GetSampleInfo(long time, QTSampleInfo& play);
		Boolean		PlaySample(QTSampleInfo& play);
		
		void		GetSize(long *width, long *height);
		long		GetMediaType();
		long		GetTimeScale();
		void		StartPlaying();
		void		StopPlaying();
		
protected:
	QTMedia*		mMedia;
	TrackHeader*	mHeader;
};

//====================================================================

typedef struct {
	long	flags;
	long	creationTime;
	long	modificationTime;
	long	timeScale;
	long	duration;
	long	preferredRate;
	short	preferredVolume;
	short	reserved[5];
	long	matrix[9];
	long	previewTime;
	long	previewDuration;
	long	posterTime;
	long	selectionTime;
	long	selectionDuration;
	long	currentTime;
	long	nextTrackID;
} MovieHeader;

//====================================================================

#define QT_ERROR -2
#define QT_NOT_A_MOVIE -3

class QTMovie {
public:
					QTMovie();
			virtual	~QTMovie();
			
		Boolean			MovieIsValid();
		
		long			ReadMoov(QTPlayer *player);
		Boolean			OpenMoov(QTPlayer *player);
		
		QTTrack*		VideoTrack();
		QTTrack*		AudioTrack();
		
		void 			GetSize(long *width, long *height);
		void 			Play();
		void 			Stop();

		long			GetDuration();
		long			GetTimeScale();
		void			SetTime(long time);
		long			GetTime();				// Current Movie Time
		void 			Task();

		long			PrepareSampleInfoList(QTSampleInfo* s, long count);
		void			FlushSampleInfo();
		void			MoreSampleInfo();
		long			PlayNext(long time);
		
		
protected:
		Byte*			mMooV;			// Movie data
		QTTrack*		mTracks;
		MovieHeader*	mHeader;
		
//		Audio and Video tracks, if present
		
		QTTrack*		mAudioTrack;
		QTTrack*		mVideoTrack;

//		Expanded audio and video samples

		QTSampleList	mAudioSamples;
		QTSampleList	mVideoSamples;
		
		int				mAudioIndex;
		int				mVideoIndex;
		long			mVideoTimeScale;
		
		long			mAudioPreroll;
		Boolean			mMute;
		
//		Time vars for playback

		long			mStartTime;
		long			mRate;
		long			mStart;
		long			mTime;
};

//====================================================================
//	QTPlayer is the i/o port of the movie
//	displays the decoded video and audio data. it also provides the 
//	user interface for controlling the movie, if any

class QTPlayer {
public:
						QTPlayer(Store *store);
	virtual				~QTPlayer();
			
			Boolean		MovieIsValid();
			
			void		GetSize(long *width, long *height);
			void		Test(Boolean start);
	
	virtual	Boolean		SetVideoBufferSize(long width,long height,long depth);
	virtual	Boolean		GetVideoBuffer(Byte **dst, long *dstRowBytes, long *depth);
	virtual	Boolean		DrawVideo();
			
	virtual	Boolean		SetAudioDesc(int sampleSize, int channels, int* frequency);
	virtual	Boolean		PlayAudio(void* sampleData, long sampleCount);
	virtual	void		StartAudio();
	virtual	void		StopAudio();
	
			void		Step(bool back, long *time, long *duration);
			void		Play();
			void		Stop();
			
//	Movie time info

			long		GetDuration();
			long		GetTimeScale();
			void		SetTime(long time);
			long		GetTime();				// Current Movie Time
			
//	Facade for data source

	virtual	long		GetLength();
	virtual	void		*GetData(long offset, long count);
	virtual	void		ReleaseData(void *data, long count);
	
protected:
			Store*		mStore;
			QTMovie*	mQTMovie;
};

#endif
