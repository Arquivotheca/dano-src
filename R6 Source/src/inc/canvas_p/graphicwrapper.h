#ifndef GRAPHICWRAPPER_H
#define GRAPHICWRAPPER_H

#include "graphic.h"
#include "graphiccanvas.h"

// Classes we refer to
class BView;

// Classes we declare in this header
class BMGraphicWrapper;
class BViewWrapper;


/*
	Interface: BMGraphicWrapper
	
	This class is meant to act as a wrapper for a single
	graphic.  Unlike a GraphicGroup, you can't add multiple
	graphics to this one.  Only one.  It supports the movement
	and drawing of the graphic by default.  Any more interesting
	behavior is to be implemented in a sub-class.
*/

class BMGraphicWrapper : public BMGraphicGroup
{
public:
			BMGraphicWrapper(BMGraphic *aGraphic, const char *name=0);

	// Graphic Movement
	virtual void	MoveTo(const float x, const float y);

	virtual void	Draw(BGraphPort *, const BRect rect=BRect(0,0,0,0));

	// Our own interface
	virtual BMGraphic *	SetGraphic(BMGraphic *newGraphic);
			BMGraphic * Graphic();
			
protected:
	
private:
};



/*

*/
/*
class BViewWrapper : public BMGraphicWrapper
{
public:
				BViewWrapper(BView *aView);
				
protected:
	BView	*fView;
	
private:
};
*/

/*
	The intention of this class is to allow you to create graphics
	like you normally would, then wrap them in a BView to live with
	other BViews.
*/

class BViewWrappedGraphic : public BMCanvas
{
public:
				BViewWrappedGraphic(BMGraphic *aGraphic);
	virtual		~BViewWrappedGraphic();
				
protected:
	BMGraphic	*fGraphic;
	
private:
};

#endif

