#ifndef _STATUSDIALOG_H_
#define _STATUSDIALOG_H_

#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Rect.h>


class StatusDialog : public BWindow
{
public:
	StatusDialog(const char *);
	
	virtual void MessageReceived(BMessage *msg);
};

class StatusDialogView : public BView
{
public:
	StatusDialogView(BRect r, const char *intxt);
	
	virtual ~StatusDialogView();
	
	virtual void Draw(BRect up);
	virtual void AttachedToWindow();
private:
	char 		*txt;
};

enum {
	F_STATUS_UPDATE =	'SUpd',
	F_STATUS_ERROR =	'SErr'
};

#endif

