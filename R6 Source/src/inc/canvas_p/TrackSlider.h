#ifndef B_TRACK_SLIDER_H
#define B_TRACK_SLIDER_H

#include "graphic.h"
#include "observable.h"
#include "islider.h"
#include "ivaluecontrol.h"

#include <Control.h>
#include <InterfaceDefs.h>


/*
	Interface: BTrackSlider
	
	A TrackSlider is your typical horizontal or vertical slider.
	It takes two graphics which represent the track and the thumb
	that slides in that track.  You can set the orientation to be
	either horzontal or vertical.
*/

class BTrackSlider : public ISlider, public IValueControl, public BObservable, public BControl 
{
public:
				BTrackSlider(BRect frame, const char *name, const char *label,
					BMessage *msg,
					BMGraphic *track, BMGraphic *thumb,
					const orientation orient,
					const float min=0.0, const float max=1.0, 
					const float defaultValue=0.0);
	virtual		~BTrackSlider();
				

	// From IValueControl
	virtual	status_t	SetValue(const BNamedData &value);
	virtual status_t	GetValue(BNamedData &value);

	// From BView
	virtual void	Draw(BRect updateRect);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(BPoint where, uint32 code, const BMessage *a_message);

	// From BControl
	virtual	status_t	Invoke(BMessage *msg = NULL);

	// From ISlider
	virtual void	SetValue(const float, const bool invoke=true);
	virtual void	SetValueBy(const BPoint, const bool invoke = true);
	virtual void	SetPosition(const float, const bool invoke = true);
	virtual void	SetPositionBy(const BPoint, const bool invoke = true);
	virtual void	SetRange(const float min, const float max);

	virtual	float	Value() const;
	virtual float	Position() const;
	virtual void	GetRange(float &min, float &max);

	virtual void	ValueChanged(const float, const bool invoke = true);
	virtual void	PositionChanged(const float, const bool invoke = true);	// Must be implemented

	// A little something of our own
	virtual void	SetActive(const bool active=true);

	// For the BObservable, this is the notification that will be sent out
	// whenever the slider is moved.
			void	SetNotification(const int32 notice) { fNotice = notice;};

protected:
	BMGraphic	*fTrackGraphic;
	BMGraphic	*fThumbGraphic;
	BMGraphicGroup	fGroup;
	BPoint		fLastPoint;

	orientation	fOrientation;
	int32		fNotice;
	bool		fIsActive;
	bool		fIsTracking;

	double		fMin;
	double		fMax;
	double		fCurrentValue;
	double		fPosition;

private:
};

#endif
