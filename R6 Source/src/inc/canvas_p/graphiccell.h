#ifndef GRAPHICCELL
#define GRAPHICCELL

#include "graphic.h"
#include "graphicwrapper.h"

#include <InterfaceDefs.h>

/*
	Interface: BGraphicCell
	
	A GraphicCell is a layout entity that allows for the layout
	of a graphic within a specified space.  The layout options
	are those defined in graphic.h for the GPosition enum

	These values correspond to the positions
	of a numeric keypad so they are easy to
	remember:
	
enum GPosition

		7  8  9
		4  5  6
		1  2  3
*/

class BGraphicCell : public BMGraphicWrapper
{
public:
				BGraphicCell(const BRect frame, const GPosition aPos = B_POSITION_CENTER, const int32 margin=0);
				BGraphicCell(BMGraphic *aGraphic, const BRect frame, const GPosition aPosition= B_POSITION_CENTER, const int32 margin=0);

	// From Graphic
	virtual void	MoveTo(const float x, const float y);

	// From GraphicGroup
	virtual void	AddChild(BMGraphic *, BMGraphic *before=0);
	virtual void	AddAfter(BMGraphic *, BMGraphic *after);

	// Inherited from GraphicWrapper
	virtual BMGraphic *	SetGraphic(BMGraphic *newGraphic);
	
protected:
	virtual void	Layout();	// Perform the layout
	
	GPosition	fPosition;		// What position does the graphic take in the cell
	int32		fMargin;
	
private:
};


class BBinaryLayout : public BMGraphicGroup
{
public:
			BBinaryLayout(BMGraphic *primary, BMGraphic *secondary, const GPosition aPos = B_POSITION_CENTER, const float margin = 0.0);
			
	virtual void	SetPrimaryGraphic(BMGraphic *aGraphic);
	virtual void	SetSecondaryGraphic(BMGraphic *aGraphic);
	virtual void	SetGraphics(BMGraphic *primary, BMGraphic *secondary);
	
protected:
	virtual void	Layout();
	
	BMGraphic	*fPrimaryGraphic;
	BMGraphic	*fSecondaryGraphic;
	GPosition	fPosition;
	float		fMargin;
	
private:
};


/*
*/

class BGraphicCellGroup : public BMGraphicGroup
{
public:
				BGraphicCellGroup(const char *name);
				
	virtual BGraphicCell *	CreateCell(const char *name, const BRect frame, const GPosition aPos=B_POSITION_CENTER, BMGraphic *aGraphic=0);
	virtual BGraphicCell *	GetCell(const char *name);
	virtual BMGraphic * 	SetGraphic(const char *cellname, BMGraphic *aGraphic);
	
protected:
private:
};

#endif
