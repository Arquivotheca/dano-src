// ===========================================================================
//	MoviePlay.cpp
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "MoviePlay.h"
#include <Debug.h>

//====================================================================

long LONG(void *l)
{
	Byte *b = (Byte *)l;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

long SHORT(void *l)
{
	Byte *b = (Byte *)l;
	return (b[0] << 8) | b[1];
}

#define PLAYAUDIO

//====================================================================

typedef struct {
	char	*name;
	long	sig;
	Boolean	leaf;
} AtomInfo;

AtomInfo gAtomInfo[] = {
	{"MovieData"				, 'mdat',true},
	{"Movie"					, 'moov',false},
	{"MovieHeader"				, 'mvhd',true},
	{"Clip"						, 'clip',true},
	{"RgnClip"					, 'crgn',true},
	{"Matte"					, 'matt',true},
	{"MatteComp"				, 'kmat',true},
	{"Track"					, 'trak',false},
	{"UserData"					, 'udta',false},
	{"TrackHeader"				, 'tkhd',true},
	{"Edits"					, 'edts',false},
	{"EditList"					, 'elst',true},
	{"Media"					, 'mdia',false},
	{"MediaHeader"				, 'mdhd',true},
	{"MediaInfo"				, 'minf',false},
	{"VideoMediaInfoHeader"		, 'vmhd',true},
	{"SoundMediaInfoHeader"		, 'smhd',true},
	{"GenericMediaInfoHeader"	, 'gmhd',true},
	{"GenericMediaInfo"			, 'gmin',true},
	{"DataInfo"					, 'dinf',false},
	{"DataRef"					, 'dref',true},
	{"SampleTable"				, 'stbl',false},
	{"STSampleDesc"				, 'stsd',true},
	{"STTimeToSamp"				, 'stts',true},
	{"STSyncSample"				, 'stss',true},
	{"STSampleToChunk"			, 'stsc',true},
	{"STShadowSync"				, 'stsh',true},
	{"Handler"					, 'hdlr',true},
	{"STSampleSize"				, 'stsz',true},
	{"STChunkOffset"			, 'stco',true},
	{"DataRefContainer"			, 'drfc',true},
	{"TrackReference"			, 'tref',true},
	{"ColorTable"				, 'ctab',true},
	{"LoadSettings"				, 'load',true},
	{"Unknown"					, 0, true}
};

//====================================================================

char *SigStr(long sig)
{
	static char str[6];
	*(long *)str = sig;
	str[4] = 0;
	return str;
}

AtomInfo *LookupAtom(long sig)
{
	short i;
	for (i = 0; gAtomInfo[i].sig; i++)
		if (sig == gAtomInfo[i].sig)
			break;
	return gAtomInfo + i;
}

char *AtomName(long sig)
{
	return LookupAtom(sig)->name;
}

Boolean AtomIsLeaf(long sig)
{
	return LookupAtom(sig)->leaf;
}

//	Get the next atom in the chain

Byte *NextAtom(Byte *atom)
{
	long atomSize = LONG(atom);
	if (atomSize < 8)
		return nil;
	return atom + atomSize;
}

//	Search Atom for a sub atom

Byte *SearchAtom(Byte *atom, long sig)
{
	long atomSize = LONG(atom);
	if (atomSize < 8)
		return nil;
	Byte *end = atom + atomSize;
	atom += 8;
	
	while (atom && atom < end) {
		if (atom == nil)
			break;
		if (sig == LONG(atom + 4))		// Found it
			return atom;
		atom = NextAtom(atom);
	}
	return nil;
}

//	Show an atoms type and size

void PrintAtom(Byte *atom)
{
	long atomSize = LONG(atom);
	long atomSig = LONG(atom + 4);
	pprint("%s %5d - %s",SigStr(atomSig),atomSize,AtomName(atomSig));
}

// Dump the contents of an atom

void DumpAtom(Byte *atom)
{
	long atomSig;
	long atomSize = LONG(atom);
	
	if (atomSize < 8)
		return;
	Byte *end = atom + atomSize;
	atom += 8;
	
	pprint("¥>");
	while (atom && atom < end) {
		atomSize = LONG(atom);
		atomSig = LONG(atom + 4);
		if (atomSize == 0)
			break;
		
		PrintAtom(atom);
		//if (AtomIsLeaf(LONG(atom + 4)))
		//	pprintHex(atom,MIN(128,atomSize));
		//else
			DumpAtom(atom);
		atom = NextAtom(atom);
	}
	pprint("<¥");
	return;
}

//====================================================================

VideoCodec::~VideoCodec()
{
}
	
Boolean	VideoCodec::PreDecompress(ImageDesc *desc, long *width, long *height)	// Codec may pad width and height
{
	*width = SHORT((void*)&desc->width);
	*height = SHORT((void*)&desc->height);
	return true;
}

Boolean	VideoCodec::Decompress(void *data, ImageDesc *desc, Byte *dst, long dstRowBytes, long depth)
{
	return true;
}

//	Cinepak codec

CinepakCodec::CinepakCodec()
{
}

CinepakCodec::~CinepakCodec()
{
}

//	Align to 4 by 4

Boolean	CinepakCodec::PreDecompress(ImageDesc *desc, long *width, long *height)
{
	*width = SHORT((void*)&desc->width) + ((4 - SHORT((void*)&desc->width)) & 0x3);
	*height = SHORT((void*)&desc->height) + ((4 - SHORT((void*)&desc->height)) & 0x3);
	return true;
}

Boolean	CinepakCodec::Decompress(void *data, ImageDesc *desc, Byte *dst, long dstRowBytes, long depth)
{
	return cpDecompress((Byte *)data,dst,dstRowBytes,mCodeBook,depth);
}

//====================================================================
//	Media handlers 'display' (or 'play') the data

MediaHndlr::MediaHndlr()
{
	mPlayer = 0;
}

MediaHndlr::~MediaHndlr()
{
}

void MediaHndlr::SetDescription(void *desc)
{
}

void MediaHndlr::Display(void *data, long count)
{
	pprint("MediaHandler::Display: %d samples",count);
}

Boolean	MediaHndlr::ProcessInChunks()
{
	return false;
}

void MediaHndlr::StartPlaying()
{
}

void MediaHndlr::StopPlaying()
{
}

//====================================================================
//	Video Media Handler

VideoCodec *GetVideoCodec(long cType)	// Should be a list of codecs somewhere....
{
	switch (cType) {
		case 'cvid':	return new CinepakCodec;
	}
	return 0;
}

VideoMediaHandler::VideoMediaHandler(QTPlayer *player) : mVideoCodec(0), mCType(0), mWidth(0), mHeight(0)
{
	mPlayer = player;
	mBaseAddr = 0;
	mRowBytes = 0;
	mDepth = 0;
}

VideoMediaHandler::~VideoMediaHandler()
{
	delete mVideoCodec;
}

void VideoMediaHandler::SetDescription(void *desc)
{
	mImageDesc = (ImageDesc *)desc;
	mImageDesc->name[mImageDesc->name[0] + 1] = 0;
	pprint(">%s, %d by %d",mImageDesc->name + 1,mImageDesc->width,mImageDesc->height);
	
//	Reset the codec if the type changed

	if (mCType != LONG((void*)&mImageDesc->cType)) {
		pprint("VideoMediaHandler::SetDescription: Getting new Codec");
		delete mVideoCodec;
		mVideoCodec = GetVideoCodec(LONG((void*)&mImageDesc->cType));
		mCType = LONG((void*)&mImageDesc->cType);
	}

//	Check if the size changed

	long width = SHORT((void*)&mImageDesc->width);
	long height = SHORT((void*)&mImageDesc->height);
	mVideoCodec->PreDecompress(mImageDesc,&width,&height);
	if (width != mWidth || height != mHeight) {
		pprint("VideoMediaHandler::SetDescription: Changing size to %d by %d",width,height);
		mPlayer->SetVideoBufferSize(width,height,SHORT((void*)&mImageDesc->depth));
		mPlayer->GetVideoBuffer(&mBaseAddr,&mRowBytes,&mDepth);				// GetVideoBuffer may not really allocate anything
		mWidth = width;
		mHeight = height;
	}
}

//	Decode and draw the video in a player

void VideoMediaHandler::Display(void *data, long count)
{
	if (mBaseAddr != 0) {
		mVideoCodec->Decompress((Byte *)data,mImageDesc,mBaseAddr,mRowBytes,mDepth);
		mPlayer->DrawVideo();
	}
}

//	Sound Media Handler

SoundMediaHandler::SoundMediaHandler(QTPlayer *player)
{
	mPlayer = player;
	mKnownFormat = true;
}

SoundMediaHandler::~SoundMediaHandler()
{
}

//	SetDescription may force a change in the playback frequency of sound

void SoundMediaHandler::SetDescription(void *desc)
{
	mSoundDesc = (SoundDesc *)desc;
	
	if (LONG((void*)&mSoundDesc->dataFormat) != 'raw ') {
		mKnownFormat = false;
		char str[6];
		strcpy(str,"XXXX");
		memcpy(str,&mSoundDesc->dataFormat,4);
		pprint("SoundMediaHandler: Unknown Sound format '%s'",str);
	} else {
		int frequency_save;
		int frequency = ((ulong)LONG((void*)&mSoundDesc->sampleRate)) >> 16;
		mPlayer->SetAudioDesc(SHORT((void*)&mSoundDesc->sampleSize),
							  SHORT((void*)&mSoundDesc->numChannels),&frequency);
		frequency_save = frequency << 16;
		mSoundDesc->sampleRate = LONG((void*)&frequency_save);
	}
}

void SoundMediaHandler::Display(void *data, long count)
{
	if (mKnownFormat)
		mPlayer->PlayAudio(data,count);
}

Boolean	SoundMediaHandler::ProcessInChunks()
{
	return true;
}

void SoundMediaHandler::StartPlaying()
{
	mPlayer->StartAudio();
}

void SoundMediaHandler::StopPlaying()
{
	mPlayer->StopAudio();
}


//====================================================================

QTSampleTable::QTSampleTable(Byte *atom)
{
	long atomSig;
	long atomSize = LONG(atom);
	
	if (atomSize < 8)
		return;
	Byte *end = atom + atomSize;
	atom += 8;
	
	while (atom && atom < end) {
		atomSize = LONG(atom);
		atomSig = LONG(atom + 4);
		if (atomSize == 0)
			break;
		
		switch (atomSig) {
			case 'stsd':	mSampleDesc = (long *)(atom + 8);		break;	// Sample Description
			case 'stts':	mTimeToSamp = (long *)(atom + 8);		break;	// Time to sample
			case 'stss':	mSynchSamp = (long *)(atom + 8);		break;	// Synch Samples
			case 'stsc':	mSampleToChunk = (long *)(atom + 8);	break;	// Sample to chunk
			case 'stsz':	mSampleSize = (long *)(atom + 8);		break;	// Sample Size
			case 'stco':	mChunkOffset = (long *)(atom + 8);		break;	// Chunk Offset
		}
		atom = NextAtom(atom);
	}
}

QTSampleTable::~QTSampleTable()
{
}

// Get sample start time and duration
// SampleCount:SampleDuration in mTimeToSamp

long QTSampleTable::TimeToSample(long *time, long *duration)
{
	long *t = mTimeToSamp + 2;
	long curTime = 0;
	long curSample = 0;
	for (long i = LONG((void*)(mTimeToSamp+1)); --i; t += 2) {
		long durations = LONG((void*)(t+0))*LONG((void*)(t+1));
		if (*time < curTime + durations)
			break;
		curTime += durations;
		curSample += LONG((void*)(t+0));
	}

	long sample = (*time - curTime)/LONG((void*)(t+1));
	*time = curTime + sample*LONG((void*)(t+1));				// Align time to start of sample
	*duration = LONG((void*)(t+1));
	
	//pprint("TimeToSample: Time: %4d, Duration: %4d, Sample: %4d",*time,*duration,sample + curSample);
	return sample + curSample;
}

//	Convert a sample index into a chunk index
//	FirstChunk:SamplesPerChunk:SampleDescription in mSampleToChunk

long QTSampleTable::SampleToChunk(long sample, long *firstSampleInChunk, long *samplesInChunk, long *descID)
{
	if (sample < 0 || sample >= GetSampleCount())
		return 0;
	
	long *c = mSampleToChunk + 2;
	long curSample = 0;
	for (long i = 0; i < LONG((void*)(mSampleToChunk+1)); i++,c += 3) {
		long samples;
		if (i == LONG((void*)(mSampleToChunk+1)) - 1)
			samples = GetSampleCount() - curSample;	// The rest of the samples
		else
			samples = (LONG((void*)(c+3)) - LONG((void*)(c+0)))*LONG((void*)(c+1));
			// Number of samples in this group		
		if (sample < curSample + samples)
			break;
		curSample += samples;
	}
	
//	Fall thru for last group

	long chunk = (sample - curSample)/LONG((void*)(c+1));			// Offset into chunk group
	*firstSampleInChunk = curSample + chunk*LONG((void*)(c+1));	// First sample in this chunk
	*samplesInChunk = LONG((void*)(c+1));							// number of samples in this chunk
	*descID = LONG((void*)(c+2));									// Descriptor ID
	return chunk + LONG((void*)(c+0));
}

//	Sample descriptions

long QTSampleTable::GetSampleDescriptionCount()
{
	return LONG((void*)(mSampleDesc+1));
}

void *QTSampleTable::GetSampleDescription(long index)
{
	if (index <= 0 || index > GetSampleDescriptionCount())
		return 0;
	Byte *desc = (Byte *)mSampleDesc + 8;
	while (--index)
		desc += LONG((void*)desc);
	return desc;
}

long QTSampleTable::GetSampleCount()
{
	return LONG((void*)(mSampleSize+2));		// This many samples
}

long QTSampleTable::GetChunkCount()
{
	return LONG((void*)(mChunkOffset+1));		// This many chunks
}

//	Get an individual sample (like a video frame)
//	time will be aligned to start of sample

Boolean QTSampleTable::GetSample(long *time, long *duration, long *offset, long *size, long *descID)
{
	long sample = TimeToSample(time,duration);			// Get sample start time and duration
	long firstSampleInChunk;							// Get chunk
	long samplesInChunk;
	long chunk = SampleToChunk(sample,&firstSampleInChunk,&samplesInChunk,descID);
	if (chunk == 0)
		return false;
		
	*offset = LONG((void*)(mChunkOffset+2+(chunk-1)));
	for (long i = firstSampleInChunk; i < sample; i++)
		*offset += LONG((void*)(mSampleSize+3+i));					// Add sample offset
	*size = LONG((void*)(mSampleSize+3+sample));
//	_sPrintf("Time:%d, duration:%d, offset:%x, size:%x, ID:%x\n",
//			 *time, *duration, *offset, *size, *descID);
	return true;
}

//	Get a chunk of samples (like a chunk of sound)
//	time will be aligned to start of chunk

Boolean QTSampleTable::GetSampleChunk(long *time, long *duration, long *count, long *offset, long *size, long *descID)
{
	long sample = TimeToSample(time,duration);					// Get sample chunk start time and duration
	long chunk = SampleToChunk(sample,&sample,count,descID);	// Get chunk
	if (chunk == 0)
		return false;
		
	*offset = LONG((void*)(mChunkOffset+2+(chunk-1)));
	*size = *count * LONG((void*)(mSampleSize+1));
	*duration *= *count;
	return true;
}

//====================================================================

long ConvertTime(long time, long from, long to)
{
	return ((time * to) + (from >> 1))/from;
}

void QTSampleInfo::Print()
{
	pprint("Time [%5d:%3d] Media (%6d:%5d)",mTime,mDuration,mOffset,mSize);
}

//====================================================================

QTSampleList::QTSampleList() : mCount(0)
{
}

void QTSampleList::Create(QTTrack* track)
{
	mCount = 0;
	long time = 0;
	while (track->GetSampleInfo(time,mList[mCount]) == true) {
		time += mList[mCount].mDuration;
		mCount++;
	}
	pprint("QTSampleList: %d samples",mCount);
}

long QTSampleList::Count()
{
	return mCount;
}

QTSampleInfo& QTSampleList::Get(int index)
{
	//NP_ASSERT(index >= 0 && index < mCount);
	return mList[index];
}

//====================================================================

QTMedia::QTMedia(Byte *atom, QTPlayer *player) : mHandler(0), mSampleTable(0), mLastDescID(0), mPlayer(player),mMediaType(0)
{
	long atomSig;
	long atomSize = LONG(atom);
	
	if (atomSize < 8)
		return;
	Byte *end = atom + atomSize;
	atom += 8;
	
	while (atom && atom < end) {
		atomSize = LONG(atom);
		atomSig = LONG(atom + 4);
		if (atomSize == 0)
			break;
		
		switch (atomSig) {
			case 'mdhd':							// Media Header
				pprint("Media Header");
				mMediaHeader = *(MediaHeader *)(atom + 8);
				break;
			case 'hdlr':							// Media Data Handler
				pprint("Media Data Handler");
				break;
			case 'minf':							// Load Media Info
				InitMediaInfo(atom);
				break;
		}
		atom = NextAtom(atom);
	}
}

QTMedia::~QTMedia()
{
	delete mHandler;
	delete mSampleTable;
}

long QTMedia::GetMediaType()
{
	return mMediaType;
}

//	Load media info including sample table

void QTMedia::InitMediaInfo(Byte *atom)
{
	long atomSig;
	long atomSize = LONG(atom);
	
	if (atomSize < 8)
		return;
	Byte *end = atom + atomSize;
	atom += 8;
	
	while (atom && atom < end) {
		atomSize = LONG(atom);
		atomSig = LONG(atom + 4);
		if (atomSize == 0)
			break;
		
		switch (atomSig) {
			case 'vmhd':							// Video Media Header
				pprint("Video Media Header");
				mHandler = new VideoMediaHandler(mPlayer);
				mMediaType = 'vmhd';
				break;
			case 'smhd':							// Sound Media Header
				pprint("Sound Media Header");
				mHandler = new SoundMediaHandler(mPlayer);
				mMediaType = 'smhd';
				break;
			case 'hdlr':							// Media Handler
				pprint("Media Handler");
				break;
			case 'stbl':							// Sample Table
				mSampleTable = new QTSampleTable(atom);
				break;
		}
		atom = NextAtom(atom);
	}
}

//	Decode the play list info for the sample at time

Boolean QTMedia::GetSampleInfo(long time, QTSampleInfo& s)
{
	s.mTime = time;	// In Media time scale
	s.mCount = 1;
	
	if (mHandler->ProcessInChunks())
		return mSampleTable->GetSampleChunk(&s.mTime,&s.mDuration,&s.mCount,&s.mOffset,&s.mSize,&s.mDescID);

	return mSampleTable->GetSample(&s.mTime,&s.mDuration,&s.mOffset,&s.mSize,&s.mDescID);
}

//	Play a sample from the play list

Boolean QTMedia::PlaySample(QTSampleInfo& s)
{
	if (mLastDescID != s.mDescID) {					// Description changed
		void *desc = mSampleTable->GetSampleDescription(s.mDescID);
		mHandler->SetDescription(desc);
		
//		Hardware may refuse to play sound at exact frequency
//		Adjust rate to suit

		if (mMediaType == 'smhd') {
			long  sRate_save;

			SoundDesc* sd = (SoundDesc *)desc;
			sRate_save = ((unsigned long)LONG((void*)&sd->sampleRate)) >> 16;
			mMediaHeader.timeScale = LONG((void*)&sRate_save);
		}
		
		mLastDescID = s.mDescID;
	}
	
	void *data = mPlayer->GetData(s.mOffset,s.mSize);
	if (data) {
		mHandler->Display(data,s.mCount);			// Display the data
		mPlayer->ReleaseData(data,s.mSize);			// And release it
	}
	return true;
}

//	Return the media time scale

long QTMedia::GetTimeScale()
{
	return LONG((void*)&mMediaHeader.timeScale);
}

void QTMedia::StartPlaying()
{
	if (mHandler == NULL)
		return;
	mHandler->StartPlaying();
}

void QTMedia::StopPlaying()
{
	if (mHandler == NULL)
		return;
	mHandler->StopPlaying();
}

//====================================================================

QTTrack::QTTrack(Byte *atom, QTPlayer *player) : mMedia(0)
{
	long atomSig;
	long atomSize = LONG(atom);
	
	if (atomSize < 8)
		return;
	Byte *end = atom + atomSize;
	atom += 8;
	
	while (atom && atom < end) {
		atomSize = LONG(atom);
		atomSig = LONG(atom + 4);
		if (atomSize == 0)
			break;
		
		switch (atomSig) {
			case 'tkhd':							// Track Header
				mHeader = (TrackHeader *)(atom + 8);
				break;
			case 'edts':							// Edit list
				pprint("Edit List");
				break;
			case 'mdia':							// Media
				mMedia = new QTMedia(atom,player);
				break;
		}
		atom = NextAtom(atom);
	}
}

QTTrack::~QTTrack()
{
	delete mMedia;
}

Boolean QTTrack::GetSampleInfo(long time, QTSampleInfo& s)
{
	if (mMedia == NULL)
		return false;
	return mMedia->GetSampleInfo(time,s);
}

Boolean QTTrack::PlaySample(QTSampleInfo& s)
{
	if (mMedia == NULL)
		return false;
	return mMedia->PlaySample(s);
}

long QTTrack::GetTimeScale()
{
	if (mMedia == NULL)
		return 0;
	return mMedia->GetTimeScale();
}

long QTTrack::GetMediaType()
{
	if (mMedia == NULL)
		return 0;
	return mMedia->GetMediaType();
}

void QTTrack::GetSize(long *width, long *height)
{
	*width = LONG((void*)&mHeader->width);
	*height = LONG((void*)&mHeader->height);
}

void QTTrack::StartPlaying()
{
	if (mMedia == NULL)
		return;
	mMedia->StartPlaying();
}

void QTTrack::StopPlaying()
{
	if (mMedia == NULL)
		return;
	mMedia->StopPlaying();
}

//====================================================================

QTMovie::QTMovie() : mTracks(0), mMooV(0), mRate(0), mAudioTrack(NULL), mVideoTrack(NULL), mMute(false)
{
}

QTMovie::~QTMovie()
{
	if (mMooV)
		FREE(mMooV);
}

Boolean QTMovie::MovieIsValid()
{
	return mTracks != NULL;	// One way to tell
}

void ThrowErr(long err,char *error)
{
	if (err != 0)
		throw error;
}

//	Read the Moov header into ram

long QTMovie::ReadMoov(QTPlayer *player)
{	
	long offset = 0;
	void *data;
	long size;
		
	size = player->GetLength();
	
	while (offset < size) {
		data = player->GetData(offset,8);	// Read atom header
		long atomSize = LONG(data);
		long atomSig = LONG((Byte *)data + 4);
		player->ReleaseData(data,8);
		
		if (atomSize < 8 || atomSize > size) {
			pprint("ReadMoov: Bad AtomSize - %d",atomSize);
			return QT_ERROR;
		}
			
		pprint("Atom '%s':%4d - %s",SigStr(atomSig),atomSize,AtomName(atomSig));
		
		if (atomSig == 'moov') {			// Read moov atom
			pprint("Reading Movie (%d bytes)",atomSize);
			mMooV = (Byte *)MALLOC(atomSize);
			if (mMooV == nil) {
				pprint("ReadMoov: Not Enough Memory for Movie");
				return QT_ERROR;
			}
			
			data = player->GetData(offset,atomSize);
			memcpy(mMooV,data,atomSize);
			player->ReleaseData(data,atomSize);
			break;
		}
		offset += atomSize;
	}

	return 0;
}

// Parse the movie for the movie header, tracks or user data

Boolean QTMovie::OpenMoov(QTPlayer *player)
{
	QTTrack* track;

	if (mMooV == nil)
		return false;
		
	Byte *atom = mMooV;
	long atomSig;
	long atomSize = LONG(atom);

	if (atomSize < 8)
		return false;
	Byte *end = atom + atomSize;
	atom += 8;
	
	while (atom && atom < end) {
		atomSize = LONG(atom);
		atomSig = LONG(atom + 4);
		if (atomSize == 0)
			break;
		
		switch (atomSig) {
			case 'mvhd':									// Movie Header
				pprint("Movie Header");
				mHeader = (MovieHeader *)(atom + 8);
				break;
			case 'trak':
				track = new QTTrack(atom,player);	// Track
				AddOrSet(mTracks,track);
				break;
			case 'udta':										// User data
				pprint("User Data");
				break;
		}
		atom = NextAtom(atom);
	}
	
//	Expand video and audio samples
//	QTSampleList expand sample info and can get quite large
//	Should probably page or something

	mMute = true;
	mAudioPreroll = 0;
	mAudioTrack = AudioTrack();
	if (mAudioTrack) {
		mAudioIndex = 0;
		mAudioSamples.Create(mAudioTrack);
	}

	mVideoTrack = VideoTrack();
	if (mVideoTrack) {
		mVideoIndex = 0;
		mVideoSamples.Create(mVideoTrack);
		mVideoTimeScale = mVideoTrack->GetTimeScale();
	} else
		mVideoTimeScale = 0;

	return true;
}

//	Lookup the Video Track, If any

QTTrack* QTMovie::VideoTrack()
{
	QTTrack *t;
	for (t = (QTTrack *)mTracks->First(); t; t = (QTTrack *)t->Next())
		if (t->GetMediaType() == 'vmhd')
			return t;
	return NULL;
}

//	Lookup the Audio Track, If any

QTTrack* QTMovie::AudioTrack()
{
	QTTrack *t;
	for (t = (QTTrack *)mTracks->First(); t; t = (QTTrack *)t->Next())
		if (t->GetMediaType() == 'smhd')
			return t;
	return NULL;
}

//	Play the next sample that comes along
//	Return the time of the next event

long QTMovie::PlayNext(long currentTime)
{
	QTSampleInfo s;
	long nextTime = 0x7FFFFFFF;
# if 0
//	Time to play next audio sample?

	if (mAudioTrack && mMute == false) {
		if (mAudioIndex == mAudioSamples.Count())	// Looped
			return 0;
		s = mAudioSamples.Get(mAudioIndex);
		if ((currentTime + mAudioPreroll) >= ConvertTime(s.mTime,mAudioTrack->GetTimeScale(),GetTimeScale())) {
			mAudioIndex++;
			s.Print();
			mAudioTrack->PlaySample(s);
			//nextTime = ConvertTime(s.mTime + s.mDuration,mAudioTimeScale,GetTimeScale());
		}
	}
#endif
//	Time to play next video sample?

	if (mVideoTrack) {
		if (mVideoIndex == mVideoSamples.Count())	// Looped
			return 0;
			
		s = mVideoSamples.Get(mVideoIndex);
//		_sPrintf("VideoScale: %d, TimeScale:%d\n", mVideoTimeScale, GetTimeScale());
//		_sPrintf("cTime:%d, Time:%d, duration:%d, offset:%x, size:%x\n",
//				 currentTime, s.mTime, s.mDuration, s.mOffset, s.mSize);
		if (currentTime >= ConvertTime(s.mTime,mVideoTimeScale,GetTimeScale())) {
			mVideoIndex++;
			//s.Print();
			mVideoTrack->PlaySample(s);
			nextTime = MIN(nextTime,ConvertTime(s.mTime + s.mDuration,mVideoTimeScale,GetTimeScale()));
		}
	}
	
	//NP_ASSERT(nextTime != 0x7FFFFFFF);
	return nextTime;			// Return time of next event
}

//	Set the movie time. Setup the audio and video track times
//	Flush samples and get ready to play

void QTMovie::SetTime(long time)
{
	mTime = time;
}

void QTMovie::GetSize(long *width, long *height)
{
	*width = 0;
	*height = 0;
	if (mTracks == NULL)
		return;
		
	QTTrack *t;
	for (t = (QTTrack *)mTracks->First(); t; t = (QTTrack *)t->Next()) {
		long twidth,theight;
		t->GetSize(&twidth,&theight);
		*width = MAX(*width,twidth);
		*height = MAX(*height,theight);
	}
}

long GetCurrentTime(long timeScale)
{
	return 0;
}

//	Start the movie playing by setting the rate and remebering the start time

void QTMovie::Play()
{
	if (mRate != 0) return;
	
	mRate = 1;
	mMute = false;
	
	mAudioIndex = 0;
	mVideoIndex = 0;
	
//	PreRoll Audio, push a second into the buffer

	long currentTime = 0;
	long startTime = 0;
	mAudioPreroll = GetTimeScale()*2;	// One second audio preroll 2 secs
	
	QTSampleInfo s;
	if (mAudioTrack && mMute == false) {
		do {
			s = mAudioSamples.Get(mAudioIndex);
			mAudioIndex++;
			mAudioTrack->PlaySample(s);
			currentTime = ConvertTime(s.mTime + s.mDuration,mAudioTrack->GetTimeScale(),GetTimeScale());
		} while ((currentTime - startTime) < mAudioPreroll);	// 1 second
		mAudioPreroll = currentTime;
		mAudioTrack->StartPlaying();	// Allow Audio to flow...
	}
	
	mStartTime = B_NOW;
	pprint("Started Playing at %d",mStartTime);
}

void QTMovie::Stop()
{
	mRate = 0;
	if (mAudioTrack)
		mAudioTrack->StopPlaying();
	mMute = true;
}

long QTMovie::GetDuration()
{
	return LONG((void*)&mHeader->duration);
}

long QTMovie::GetTimeScale()
{
	return LONG((void*)&mHeader->timeScale);
}

// Current Movie Time

long QTMovie::GetTime()
{
	return ConvertTime(B_NOW-mStartTime,1000,GetTimeScale());
}

void QTMovie::Task()
{
}

//====================================================================
//	Player is the focal point of the data coming out of the window

QTPlayer::QTPlayer(Store *store) : mStore(store)
{
	mQTMovie = new QTMovie;
	mQTMovie->ReadMoov(this);	// Read movie structures into ram
	mQTMovie->OpenMoov(this);	// Interpret those structures, get ready to play
}

QTPlayer::~QTPlayer()
{
	delete mQTMovie;
}

Boolean QTPlayer::MovieIsValid()
{
	return mQTMovie->MovieIsValid();
}

void QTPlayer::GetSize(long *width, long *height)
{
	mQTMovie->GetSize(width,height);
}

void QTPlayer::Step(bool back, long *time, long *duration)
{
	if (*time == -1)
		*time = 0;
	long next = mQTMovie->PlayNext(*time);	// next is the next sample time, may be be before *time
	if (next < *time)
		*duration = 0;	// Play another as soon as possible
	else {
		*duration = next - *time;
		*time = next;
	}
	
	if (next == 0) {
		mQTMovie->Stop();
		mQTMovie->Play();
		*time = 0;
	}
}

long QTPlayer::GetDuration()
{
	return mQTMovie->GetDuration();
}

long QTPlayer::GetTime()
{
	return mQTMovie->GetTime();
}

long QTPlayer::GetTimeScale()
{
	return mQTMovie->GetTimeScale();
}

Boolean	QTPlayer::SetVideoBufferSize(long width,long height,long depth)
{
	return false;
}

Boolean	QTPlayer::GetVideoBuffer(Byte **dst, long *dstRowBytes, long *depth)
{
	*dst = 0;
	*dstRowBytes = 0;
	*depth = 0;
	return false;
}

Boolean	QTPlayer::DrawVideo()
{
	return false;
}

Boolean	QTPlayer::SetAudioDesc(int sampleSize, int channels, int* frequency)
{
	pprint("QTPlayer SetAudioInfo - Size:%d, Channels: %d, Frequency: %d",sampleSize,channels,frequency);
	return false;
}

Boolean	QTPlayer::PlayAudio(void *sampleData, long sampleCount)
{
	pprint("QTPlayer PlayAudio - %d",sampleCount);
	return false;
}

void QTPlayer::StartAudio()
{
}

void QTPlayer::StopAudio()
{
}

long QTPlayer::GetLength()
{
	return mStore->GetLength();
}
	
void* QTPlayer::GetData(long offset, long count)
{
	return mStore->GetData(offset,count);
}

void QTPlayer::ReleaseData(void *data, long count)
{
	mStore->ReleaseData(data,count);
}

void QTPlayer::Play()
{
	mQTMovie->Play();
}

void QTPlayer::Stop()
{
	mQTMovie->Stop();
}

