#ifndef HIGHLITES_H
#define HIGHLITES_H

#include "interactor.h"
#include "graphic.h"

//==========================================================
// Class Interface: BMAbsHighlitable
//
// This is an abstract base class meant to serve as the protocol
// for graphics that want to do highliting in response to 
// being hovered over by a mouse selected by keyboard navigation.
// Things like switches and pushbuttons will want to inherit
// from this and implement the Highlite() method appropriately.
//==========================================================

class BMAbsHighlitable
{
public:
					BMAbsHighlitable();
	virtual 		~BMAbsHighlitable();
					
	virtual void	Highlite(BGraphPort *, const bool showHighlite=true);
	
	virtual bool	SetHighlited(const bool highlited=true);
	
			bool	IsHighlited() const;

	virtual const char * ShortDescription();
	virtual const char * LongDescription();

protected:
	
	bool	fIsHighlited;
		
private:
};

//==========================================================
// Class Interface: BMHighlite
//
// This is a concrete implementation of the BMAbsHighlite 
// protocol for a graphic.  It is meant to take a graphic
// and put some highliting around it when the mouse is
// moved over it.
//==========================================================

/*
class BMHighlite : public BMGraphicGroup, public BMMouseTracker, public BMAbsHighlitable
{
public:
				BMHighlite(BMGraphic *);
				
	// From BMGraphic
	virtual void	Draw(BGraphPort *, const BRect rect);

	// From BMAbsHighlitable
	virtual void	Highlite(BGraphPort *, const bool showHighlite=true);

protected:
	BMGraphic *fGraphic;
	
private:
};
*/

#endif
