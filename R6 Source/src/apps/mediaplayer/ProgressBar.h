#ifndef _PROGRESS_BAR_H
#define _PROGRESS_BAR_H

#include <View.h>
#include <Font.h>

class ProgressBar : public BView {
public:
	ProgressBar(BRect rect, const char *name, uint32 resizingMode);
	void SetValue(float, bool lowWater);
	
protected:
	virtual void Draw(BRect);	
private:
	float fValue;
	bool fLowWater;
	float fMessageWidth;
	font_height fFontMetrics;
};


#endif

