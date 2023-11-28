//
// AudioWrapperDataSource.h
//
//   A CDDataSource that can apply some transformations to a separate audio
//   CDDataSource.  Possible adjustments are changing the beginning and end
//   of the track, modifying the gain, and applying a fade-in and fade-out
//   to the underlying CDDataSource.
//
//   NOTE: Because there is no notification mechanism for when a CDDataSource
//   changes lengths, undesirable things may happen if you layer this on top
//   of another CDDataSource that can change lengths, or layer another CDDataSource
//   on top of this one.  If you want to do things like this, you should be aware
//   of the synchronization problem when utilizing the SetStart/SetEnd functions, and
//   work around them.
//
//   by Nathan Schrenk (nschrenk@be.com)
//

#ifndef _AUDIO_WRAPPER_DATA_SOURCE_H_
#define _AUDIO_WRAPPER_DATA_SOURCE_H_

#include "CDDataSource.h"

class AudioWrapperDataSource : public CDDataSource
{
public:
						AudioWrapperDataSource(CDDataSource *realSource);
						AudioWrapperDataSource(BMessage *archive);
	virtual				~AudioWrapperDataSource();
	
	// CDDataSource functions
	virtual status_t	InitCheck();
	virtual status_t	Read(void *data, size_t len, off_t posn);
	virtual size_t		Length(void);
	virtual bool		IsAudio();
	virtual char 		*Description();
	
	// BArchivable functions
	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;
	
	// new functions
	void				SetGain(float gain);
	float				Gain();
	void				SetGainEnabled(bool enabled);
	bool				GainEnabled();

	void				SetStart(uint64 offset);
	void				SetEnd(uint64 offset);
	void				SetFadeIn(uint32 length);
	void				SetFadeOut(uint32 length);

	uint64				GetStart();
	uint64				GetEnd();
	uint32				GetFadeIn();
	uint32				GetFadeOut();
	
	CDDataSource*		Source();
	
private:
	void				Init(CDDataSource *source);
	static void			ApplyGain(int16 *data, size_t len, float beginGain, float endGain);
	
	CDDataSource		*fSource;
	uint32				fFadeInLength;	// fade in relative to start
	uint32				fFadeOutLength;	// fade out relative to end
	uint64				fStartOffset;	// the first valid index
	uint64				fEndOffset;		// one past the last valid index
	float				fGain;			// gain to be applied, from 0.0 to 2.0
	bool				fGainEnabled;	// whether or not to apply gain during Read()
};
	
#endif // _AUDIO_WRAPPER_DATA_SOURCE_H_
