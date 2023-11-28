#ifndef LAYOUT_H
#define LAYOUT_H

#include "graphic.h"
#include <InterfaceDefs.h>

class BMLinearLayout : public BMGraphicGroup
{
public:
			BMLinearLayout(const char *name=0, const float gap=1.0, 
				const float leftMargin = 0.0, 
				const GAlignment linement = B_GALIGN_NONE);
			
	// Modify these from BMGraphic
	virtual BRect	Frame();
	
	// Modify these from BMGraphicGroup
	virtual void	AddChild(BMGraphic *, BMGraphic *before=0);
	virtual void	AddAfter(BMGraphic *, BMGraphic *after);
	virtual bool	Remove(BMGraphic *);

	// These are horizontal layout specific
	virtual void	SetGap(const float);
	virtual void	SetMargin(const float);
	virtual void	SetAlignment(GAlignment);
	
protected:
	virtual void	Layout()=0;
	
	float		fGap;
	float		fMargin;
	GAlignment	fAlignment;
	
private:
};


class BMGHLayout : public BMLinearLayout
{
public:
			BMGHLayout(const char *name=0, const float gap=1.0, 
				const float leftMargin = 0.0, 
				const GAlignment linement = B_GALIGN_NONE);
	
protected:
	virtual void	Layout();
	
private:
};


//=========================================================
//
//=========================================================
class BMGVLayout : public BMLinearLayout
{
public:
			BMGVLayout(const char *name=0, const float gap=1.0, 
				const float aMargin = 0.0, const GAlignment linement = B_GALIGN_NONE);
				
protected:
	virtual void	Layout();
	
private:
};


#endif
