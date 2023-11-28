#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <Point.h>
#include <Rect.h>
#include <List.h>
#include <String.h>

#include "simplelist.h"
#include "ggraphport.h"

// Things needed in here
class BView;
class BMCanvas;
class BGraphPort;
class BWindow;

// Things defined in here
class BDrawable;
class BTransformable;
class BMGraphic;
class BMGraphicRoster;
class BMGraphicGroup;

// Graphic Alignments
// They are done such that you could | together a vertical
// and horizontal alignment in one shot, but not multiples of each
enum GAlignment
{
	B_GALIGN_NONE = 0L,
	B_GALIGN_LEFT = 0x01,
	B_GALIGN_CENTER = 0x02,
	B_GALIGN_RIGHT = 0x04,
	B_GALIGN_TOP = 0x10,
	B_GALIGN_MIDDLE = 0x20,
	B_GALIGN_BOTTOM = 0x40
};

/*
	These values correspond to the positions
	of a numeric keypad so they are easy to
	remember:
	
		7  8  9
		4  5  6
		1  2  3
*/
enum GPosition
{
	B_POSITION_CENTER = 5L,
	B_POSITION_LEFT = 0x04,
	B_POSITION_TOP = 0x08,
	B_POSITION_RIGHT = 0x06,
	B_POSITION_BOTTOM = 0x02,
	B_POSITION_TOPLEFT = 0x07,
	B_POSITION_TOPRIGHT = 0x09,
	B_POSITION_BOTTOMLEFT = 0x01,
	B_POSITION_BOTTOMRIGHT = 0x03
};

// Run directions
enum Direction
{
	B_GDIRECTION_LEFTRIGHT,
	B_GDIRECTION_RIGHTLEFT,
	B_GDIRECTION_TOPBOTTOM,
	B_GDIRECTION_BOTTOMTOP
};

/*
	Various messages that graphics might think about
	sending out in various instances.
*/

enum EGraphicChanges
{
	B_GRAPHIC_WILL_MOVE = 0x500L,
	B_GRAPHIC_DID_MOVE,
	B_GRAPHIC_WILL_RESIZE,
	B_GRAPHIC_DID_RESIZE,
	B_GRAPHIC_WILL_ADD,
	B_GRAPHIC_DID_ADD,
	B_GRAPHIC_WILL_REMOVE,
	B_GRAPHIC_DID_REMOVE
};

/*

*/
class BDrawable
{
public:
	// Graphic Drawing
	virtual void	Draw(BView *, const BRect rect=BRect(0,0,0,0))=0;
	virtual void	Draw(BGraphPort *, const BRect rect=BRect(0,0,0,0))=0;
	virtual void	DrawAt(BGraphPort *, const BPoint origin=BPoint(0,0), const BRect updateRect=BRect(0,0,0,0))=0;
	virtual void	DrawAt(BView *, const BPoint origin=BPoint(0,0), const BRect updateRect=BRect(0,0,0,0))=0;

protected:
private:
};

/*

*/

class BTransformable
{
public:
	virtual void	MoveTo(const float x, const float y)=0;	// This is the one you need to implement
	virtual void	MoveBy(const float x, const float y)=0;	// This is the one you need to implement

protected:
private:
};

/*

*/

class BGraphicHierarchy
{
public:
	// Our own little additions
	virtual void	AddChild(BMGraphic *child, BMGraphic *before = 0)=0;
	virtual void	AddAfter(BMGraphic *, BMGraphic *after)=0;
	virtual bool	Remove(BMGraphic *)=0;
	virtual void	RemoveAll()=0;

	// Hit detection
	// Find which graphic is in the group at a particular
	// location.
	virtual int32		CountChildren() =0;
	virtual BMGraphic	* GraphicAt(int32 index)=0;
	virtual BMGraphic	* GraphicAt(const BPoint)=0;
	virtual BMGraphic	* GraphicNamed(const char *graphicName)=0;
	virtual void		GetGraphicsAt(const BPoint, List<BMGraphic*> *)=0;
	virtual bool		Contains(const BPoint)=0;

protected:
private:
};

/*

*/

class BMGraphic : public BDrawable, public BTransformable
{
public:
			BMGraphic(const char *name=0, const BRect &frame=BRect(0,0,0,0));
	
	// From BDrawable
	virtual void	Draw(BView *, const BRect rect=BRect(0,0,0,0));
	virtual void	Draw(BGraphPort *, const BRect rect=BRect(0,0,0,0));
	virtual void	DrawAt(BView *, const BPoint origin=BPoint(0,0), const BRect updateRect=BRect(0,0,0,0));
	virtual void	DrawAt(BGraphPort *, const BPoint origin=BPoint(0,0), const BRect updateRect=BRect(0,0,0,0));
			
	// From BTransformable
	virtual void	MoveTo(const float x, const float y);	// This is the one you need to implement
	virtual void	MoveBy(const float x, const float y);	// This is the one you need to implement

	// Convenient additions to transformable
	virtual void	MoveTo(const BPoint);				// These two are for convenience
	virtual void	MoveBy(const BPoint);
	
	// Particular to BMGraphic
	virtual BRect	Frame() ;
	virtual void	SetFrame(const BRect);
	virtual BRect	Bounds() ;
	virtual BRect	PreferredSize() ;
	
	// Hit detection
	virtual bool	Contains(const BPoint) ;
	
			const BString&	GraphicName() const { return fName;};

protected:			
	BString	fName;
	BRect	fFrame;
	BGraphPort	*fLastCanvas;
	BView	*fLastDrawView;
	
private:
};



// Forward declaration of list object
template <class T> class simplelist;
template <class T> class List;



class BMGraphicGroup : public BMGraphic, public BGraphicHierarchy
{
public:
			BMGraphicGroup(const char *name=0, const BRect &frame=BRect(0,0,0,0));
	virtual ~BMGraphicGroup();
	
	// Graphic Movement
	virtual void	MoveTo(const float x, const float y);

	// Our own little additions
	virtual void	AddChild(BMGraphic *, BMGraphic *before = 0);
	virtual void	AddAfter(BMGraphic *, BMGraphic *after);
	virtual bool	Remove(BMGraphic *);
	virtual void	RemoveAll();
	
	// Hit detection
	// Find which graphic is in the group at a particular
	// location.
	virtual int32		CountChildren() ;
	virtual BMGraphic	* GraphicAt(int32 index);
	virtual BMGraphic	* GraphicAt(const BPoint);
	virtual BMGraphic	* GraphicNamed(const char *graphicName);
	virtual void	GetGraphicsAt(const BPoint, List<BMGraphic*> *);
	virtual bool	Contains(const BPoint) ;
	
	// By default, drawing will happen using a painter's 
	// algorithm, that is, the 'back' is the first element
	// in the list, and is thus drawn first.
	virtual void	Draw(BGraphPort *, const BRect rect=BRect(0,0,0,0));
	virtual void	Draw(BView *, const BRect rect=BRect(0,0,0,0));


	virtual void	SetWindow(BWindow *);
	virtual BWindow	*Window();
	
protected:
	virtual void	DrawChildren(BGraphPort *, const BRect rect);
	virtual void	DrawChildren(BView *, const BRect rect);

	simplelist<BMGraphic *> fList;
	BWindow		*fWindow;
	
private:
	BMGraphicGroup();
	BMGraphicGroup(const BMGraphicGroup &);
};



//==========================================================
//
//==========================================================

class BMGraphicRoster
{
public:
	// Graphic cache management
	// Having a cache allows you to lookup a graphic
	// by name once it is created.
	static void		AddGraphic(const char *, BMGraphic *);
	static BMGraphic	*GetGraphic(const char *name);
	
	// don't call this
	static BList	*GraphicCache();
};

#endif
