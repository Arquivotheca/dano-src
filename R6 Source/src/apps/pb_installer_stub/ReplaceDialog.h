#ifndef _REPLACEDIALOG_H_
#define _REPLACEDIALOG_H_

#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Rect.h>

// ReplaceDialog.h

// type codes
const uchar ASK_ALWAYS			= 0;
const uchar ASK_IF_VERSION		= 1;
const uchar ASK_IF_CREATION		= 2;
const uchar ASK_IF_MODIFICATION	= 3;

class ReplaceDialog : public BWindow
{
public:
	ReplaceDialog(	int32	*_replOption,
					bool	*_applyToAll,
					char	*_replText,
					uchar	_type);

	virtual void	MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();
	long			Go();
private:
	int32	*replOption;
	bool	*applyToAll;
	uchar type;
};


enum {
	M_RADIO_SET	=	'RaSe',
	M_DONT_ASK =	'DoAs'
};


class ReplaceView : public BView
{
public:
	ReplaceView(	BRect	frame,
					short	_replOption,
					char	*_replText,
					uchar _type );

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect up);
private:
	short	replOption;
	char	*replText;
	uchar	type;
};


#endif
