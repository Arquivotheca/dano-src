#ifndef __DIGIT_VIEW__
#define	__DIGIT_VIEW__

#include <View.h>

// Possible desirable layouts:
// Number
//	0, 00, 000 ...
//  0.0, 0.00 ...
// Time
//  00:00  00:00.0 ...
//  00:00:00 00:00:00.0
//  0 00:00:00.0
//
// Right now, only the first is implemented

class DigitView : public BView
{
public:
					DigitView(int32 digits);
	virtual			~DigitView();

	virtual	void	Draw(BRect updateRect);

			void	SetValue(int32 value)		{mValue = value;}
private:
	int32			mValue;
	int32			mDigits;
	BBitmap*		mDigitBits[10];
};

#endif /*__DIGIT_VIEW__*/
