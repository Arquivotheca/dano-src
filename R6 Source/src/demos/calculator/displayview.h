//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#ifndef _CDISPVIEW_H_
#define _CDISPVIEW_H_

#include <interface/View.h>

enum {
	OP_SIN = 'oper',
	OP_COS,
	OP_TAN,
	OP_POW,
	OP_PI,
	OP_PLUS,
	OP_MINUS,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_EQUALS
};

class DisplayView : public BView {
	public:
							DisplayView(BRect frame);
		virtual void		Draw(BRect updateRect);
		
		void				SetString(char *s);
		void				AddDigit(char d);
		void				SetValue(double f);
		double				GetValue(void);
		void				Operation(int32 op);
		void				Clear(void);
		
		void				Copy(void);
		void				Paste(void);

	private:
		char 				*displayOp;
		char 				displayString[16];
		double				displayValue;		// Math results
		int32 				displayHeight;
		int32				displayWidth;
		int32				displayBaseline;
		
		double				prevValue;
		int32				operation;			// Operation to perform
		bool				prevKeyWasOp;		// True if last key was an op
		bool				prevOpWasEquals;
};

#endif
