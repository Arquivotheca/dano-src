/*--------------------------------------------------------------------*\
  File:      ProgressBar.h
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
\*--------------------------------------------------------------------*/

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <View.h>
#include <StatusBar.h>
#include <Bitmap.h>


class ProgressBar : public BView
{
public:

	// Public member functions		
	ProgressBar(const BRect a_frame, const char * const a_name);
	virtual ~ProgressBar(void);
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect);
	
	rgb_color BarColor() const
		{ return m_status_bar->BarColor(); }

	float CurrentValue() const
		{ return m_status_bar->CurrentValue(); }
	float MaxValue(void) const
		{ return m_status_bar->MaxValue(); }
	virtual void SetMaxValue(const float a_val);
	
	
	virtual void Reset(void);	
	virtual void Update(const float a_delta);

private:

	typedef BView Inherited;
	
	// Private data members
	void RedrawStatus();
	BStatusBar *m_status_bar;
	BBitmap *m_bitmap;
	float m_delta;
	bigtime_t m_last_update;
};

const int32 kBarberPoleBitmapWidth = 196;
const int32 kBarberPoleBitmapHeight = 13;

class BarberPoleView : public BView {
public:
	BarberPoleView(BRect, const char *);

protected:
	virtual	void Draw(BRect);
	virtual void Pulse();

	void DrawCurrentBarberPole();
private:
	BBitmap bitmap;	
	const unsigned char *const *bits;
	int32 count;
	int32 indx;
};

#endif    // PROGRESS_BAR_H
