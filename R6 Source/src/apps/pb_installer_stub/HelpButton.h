#ifndef _HELPBUTTON_H_
#define _HELPBUTTON_H_

#include <PictureButton.h>
#include <Rect.h>
#include <Message.h>
#include <View.h>


// HelpButton.h

class HelpButton : public BPictureButton
{
public:
					HelpButton( BRect frame,
								ulong resizeMask,
								const char *_helpTitle,
								const char *_helpText);
virtual				~HelpButton();
virtual void		MessageReceived(BMessage *);
virtual void		AttachedToWindow();
virtual void		Show();
virtual void		Hide();
		void		SetHelpText(const char *txt);
//		char		*HelpText();

static void			InitPicture(BView *);
static void			Free();
	static BPicture *HelpButtonPicture;
	static BPicture *HelpButtonOnPicture;
private:
	const char		*helpText;
	const char 		*helpTitle;
	BMessenger		helpWindowMessenger;
	static bool		picsInited;
};


enum {
	M_HELP_REQUESTED =  'Help',
	M_DO_ACTIVATE =		'DoAc',
	M_SET_HTEXT =		'SHTx'
};

#endif

