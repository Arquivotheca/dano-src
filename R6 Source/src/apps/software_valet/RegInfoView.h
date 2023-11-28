// RegInfoView.h

#ifndef _REGINFOVIEW_H_
#define _REGINFOVIEW_H_

#include <View.h>
#include <Message.h>


class RegInfoView : public BView
{
public:
	RegInfoView(BRect frame, BMessage *, BMessage *,
			const char *serialValue = NULL);
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
private:
	BMessage *inMsg;
	BMessage *outMsg;
	
	const char	*fSerial;
};

enum {
	S_REGINFO		= 'SRIn'
};

#endif
