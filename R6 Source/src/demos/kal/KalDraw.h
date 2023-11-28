#ifndef __KalDraw
#define __KalDraw

#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#include "KalView.h"

enum {dStart, dChangeSize, dQuit};

class KalDraw : public BLooper {

public:
				KalDraw (KalView *view);
		void	ChangeSize (void);
virtual	void	MessageReceived (BMessage* msg);
virtual	void	Quit ();

	int32		pending_message_count;

private:
		int		Random (int min, int max);
		void	DrawLines (void);
};

#endif
