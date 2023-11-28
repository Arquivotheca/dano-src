#ifndef ISLIDER_H
#define ISLIDER_H

/*
	Class: ISlider

	This is a protocol specification for sliders that are
	continuous.  It is intended to be inherited by a BControl
	so that it can exhibit slider behavior.
*/

class ISlider
{
	virtual void	SetValue(const float, const bool invoke=true)=0;
	virtual void	SetValueBy(const BPoint, const bool invoke)=0;
	virtual void	SetPosition(const float, const bool invoke)=0;
	virtual void	SetPositionBy(const BPoint, const bool invoke)=0;
	virtual void	SetRange(const float min, const float max)=0;
	
	virtual float	Value() const = 0;
	virtual float	Position() const =0;
	virtual void	GetRange(float &min, float &max)=0;
	
	virtual void	ValueChanged(const float, const bool invoke = true)=0;
	virtual void	PositionChanged(const float, const bool invoke)=0;	// Must be implemented

protected:
	
private:
	
};

#endif
