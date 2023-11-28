
#ifndef CDCUESHEET_H
#define CDCUESHEET_H

#include <OS.h>

class CDTrack;

struct CDCueLine
{
	uchar ctl_adr;
#define CTL_AUDIO_2CH     0x00
#define CTL_AUDIO_4CH     0x80
#define CTL_AUDIO_2CHP    0x10
#define CTL_AUDIO_4CHP    0x90
#define CTL_DATA          0x40
#define CTL_COPY_PERMIT   0x20
#define ADR_TNO_IDX       0x01
#define ADR_CATALOG       0x02
#define AD_ISRC           0x03

	uchar trackno;
	/* min len is 4 sec (300 blocks) */
	
	uchar index;
	uchar data_form;
	
	uchar scms;
	uchar min;
	uchar sec;
	uchar frame;
};


class CDCueSheet
{
public:
	CDCueSheet(CDTrack *tracks);
	~CDCueSheet(void);
	
	void SetTracks(CDTrack *tracks);
	CDCueLine *Lines(void);
	uint LineCount(void);
	
private:
	CDCueLine *lines;
	uint linecount;
};


#endif

