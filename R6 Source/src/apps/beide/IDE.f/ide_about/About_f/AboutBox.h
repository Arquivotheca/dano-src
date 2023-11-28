//	AboutBox.h

#ifndef _ABOUTBOX_H
#define _ABOUTBOX_H

#include <Window.h>
#include "VideoCoder.h"

class MemArea;
class AboutView;
class MAIFFPlayer;


class AboutBox :
	public BWindow
{
	friend class AboutView;
public:
								AboutBox(
									uint32 type,
									int32 id);
								~AboutBox();

		bool					QuitRequested();

protected:

static	int32					ThreadFunc(
									void *data);

volatile	bool				fRunning;
		sem_id					fQuitted;

		void					AnimateFrame();
		void					Transition();
		void					Scroll();

		void					DoRun();

		void					PlayStampsound();

		MemArea *				fArea;
		VideoCoder<MemArea> *	fVideo;

		int						fWidth;
		font_height				fFontInfo;

		BBitmap *				fBackground;
		BBitmap *				fBuffer;
		BView *					fBufferView;
		BBitmap *				fText;
		BView *					fTextView;

		int						fStage;		//	AboutView and DoRun have knowledge of this
		const char *			fMessage;
		float					fScroll;

		AboutView *				fWindowView;

		char *					fStampsoundData;
		int32					fStampsoundDataLength;
		MAIFFPlayer*			fSoundPlayer;
};


//	message sent by about box when closed
#define BYE_BYE_BOX	'!abo'

#endif
