#if ! defined SLIDERS_INCLUDED
#define SLIDERS_INCLUDED

#include <Slider.h>
#include <Message.h>
#include <Control.h>

// the time slider displays the time
class TimeSlider : public BSlider
{
	BSlider		*drawonupdate1;
	BSlider		*drawonupdate2;
	BStringView	*labeltarget;

public:
	TimeSlider(BRect frame, const char *name,
		const char *label, BStringView *target, BMessage *message);

	void	DrawThumb();
	void	DrawText();
	int32	Time();
	void	SetTime(int32 t);
	void	SetDrawOnUpdate(BSlider *d1, BSlider *d2);
};

class RunSlider : public TimeSlider
{
	BSlider *a;
	BSlider *b;

public:
	RunSlider(BRect frame, const char *name,
		const char *label, BStringView *target, BMessage *message);

	virtual void SetValue(int32 value);
	void SetUpper(BSlider *_a, BSlider *_b);
};

class UpperSlider : public TimeSlider
{
	BSlider	*limit;
	BSlider *a;
	BSlider *b;
public:
	UpperSlider(BRect frame, const char *name,
		const char *label, BStringView *target, BMessage *message);

	virtual void SetValue(int32 value);
	void	SetLower(BSlider *_a, BSlider *_b);
	void	SetLimit(BSlider *s);
	void	DrawBar();
};

#endif
