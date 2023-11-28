
#ifndef _NUM_CONTROL_H_
#define _NUM_CONTROL_H_

#include <Control.h>
#include <TextControl.h>

class NumControl : public BControl
{
	public:
								NumControl(BRect rect, const char *name, BMessage *msg,
									int32 startNum = 0, int32 maxNum = 0, int32 minNum = 0);
		virtual					~NumControl();
		
		void					Draw(BRect updateRect);
		void					MessageReceived(BMessage *msg);
	
		void					MouseDown(BPoint where);
		void					KeyDown(const char *bytes, int32 numBytes);
	
		void					SetLimits(int32 minimum, int32 maximum);
		void					AttachedToWindow();
		void					SetValue(int32 value);
	
	private:
		int32					fMax;
		int32					fMin;
		BTextControl *			fPageNum;
};

#endif
