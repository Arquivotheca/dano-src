#ifndef GRAPHICCANVAS_H
#define GRAPHICCANVAS_H

#include <View.h>

#include "graphic.h"

// Things we need
class BGraphPort;
class BMInteractiveGraphicGroup;

//===========================================================
// Class: BMCanvas
//
// Implementation of a general media interface 
//===========================================================

class BMCanvas : public BView 
{
public:
			BMCanvas(BRect frame, BGraphPort *aPort=0);
	virtual ~BMCanvas();
	
	virtual void	Transform(const BMatrix2D &aTransform);
	virtual void	SetFrame(const BRect aRect);
	
	// BHandler things
	virtual void	MessageReceived(BMessage *msg);
	
	// BView things
	virtual void	AttachedToWindow();
	virtual	void	AllAttached();
	virtual void	AllDetached();
	
	virtual	void	Draw(BRect updateRect);
	
	// Mouse movement
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(	BPoint where,uint32 code, const BMessage *a_message);
	
	// Some of our own additions
	virtual void	AdoptGraphic(BMGraphic *);
	virtual void	RemoveGraphic(BMGraphic *);
	virtual void	ForceDraw(BRect updateRect);
	
	virtual BGraphPort	*	GraphPort();
	virtual BMInteractiveGraphicGroup	*RootGraphic();
	
protected:
	
	BMInteractiveGraphicGroup	*fRootGraphic;
	BGraphPort					*fGraphPort;
	BMatrix2D					fTransform;
	BMatrix2D					fInverseTransform;
	BRect						fOriginFrame;
	
private:
};

#endif
