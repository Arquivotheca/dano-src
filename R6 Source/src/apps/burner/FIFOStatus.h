//
// FIFOStatus.h
//

#ifndef _FIFO_STATUS_H_
#define _FIFO_STATUS_H_

#include <View.h>

class FIFOStatus : public BView
{
public:
					FIFOStatus(BRect frame, uint32 resizingMode);
	virtual			~FIFOStatus();

	void			SetPercentFull(float percent);
	
protected:
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect update);

private:

	float			fPercentFull;
	font_height		fFontHeight;
};

#endif // _FIFO_STATUS_H_
