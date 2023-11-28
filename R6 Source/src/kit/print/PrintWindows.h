// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_WINDOWS_H_
#define _PRINT_WINDOWS_H_

#include <Window.h>
#include <MessageFilter.h>
#include <View.h>
#include <Message.h>

class BButon;
class BPrintConfigView;


namespace BPrivate
{


class ControlView : public BView
{
public:
	enum
	{
		ENABLE_OK	= 'okon',
		DISABLE_OK	= 'okof'
	};

			ControlView();
	virtual void AttachedToWindow();
	virtual void GetPreferredSize(float *width, float *height);
	virtual void LayoutChildren();
	virtual void MessageReceived(BMessage *message);

private:

	class MsgFilter : public BMessageFilter
	{
		public:
			MsgFilter(BHandler *h) : BMessageFilter(B_PROGRAMMED_DELIVERY, B_LOCAL_SOURCE), fHandler(h) { }
			filter_result Filter(BMessage *message, BHandler **target) {
				if ((message->what == ENABLE_OK) || (message->what == DISABLE_OK))
					*target = fHandler;
				return B_DISPATCH_MESSAGE;
			}
		private:
			BHandler *fHandler;
	};

	BButton *fOkButton;
	BButton *fCancelButton;
};


class BSimpleWindow : public BWindow
{
public:
			BSimpleWindow(const char *title, BPrintConfigView *view);
	status_t Go();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *);
	
private:
	sem_id fAsyncSem;
	status_t fReturnValue;
	BPrintConfigView *fUserView;
};




}

using namespace BPrivate;

#endif

