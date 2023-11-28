#ifndef _DVA_H
#define _DVA_H

#include <support/SupportDefs.h>

struct dif_sequence;

class DVAudioEncoder {
public:
				DVAudioEncoder();
				DVAudioEncoder(uchar bits, uint32 audiorate, uchar channels);
				~DVAudioEncoder();
				
	status_t	InitCheck(void) { return fInitCheck; }

	status_t	SetParameters(uchar bits, uint32 audiorate, uchar channels);

	status_t	InitializeFrame(void *frame, bigtime_t t);
	void		CompletedFrame();
	// This is assumed to be called in order of ascending time
	int32		AddAudioToFrame(bigtime_t t, int16 *audio, int32 samples,
						int32 endianness);
private:
	void		ClearAudio(int32 start, int32 end);

	status_t	fInitCheck;

	/* Target parameters */
	uchar		fBitsPerSample;		// 12, 16, 20
	uint32		fAudioRate;			// 32000, 44100, 48000 Hz
	uchar		fChannels;

	/* Frame state */
	dif_sequence	*fFrame;
	bigtime_t	fFrameAudioStartTime;
	uint32		fFrameNumber;
	bool		fPAL;

	int32		fTotalFrameSamples;
	int32		fCurrentSamples;
	int32		fMinimumSamples;
	int32		fMaximumSamples;
};

#endif // _DVA_H
