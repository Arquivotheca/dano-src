
#ifndef STRINGLABEL_H
#define STRINGLABEL_H

#include <Point.h>
#include <View.h>
#include <Font.h>

#include "graphic.h"

//==========================================================
// Class: AStringLabel
//
// This class serves as a lightweight way of drawing a string into any 
// view.  It is more lightweight than using a View, and it has more 
// parameters for displaying the text than a BStringView.
//==========================================================

class AStringLabel : public BMGraphic
{
public:
				AStringLabel(const char *aString, const BFont &, 
					const BPoint base=BPoint(-1.0,-1.0), const bool debugging=false);
				AStringLabel(const char *aString, const BPoint base=BPoint(-1.0,-1.0), 
					const float fontSize=12.0, const BFont *font=be_plain_font, const bool debugging=false);

	virtual 	~AStringLabel();
		
	virtual void	Draw(BGraphPort* aView, const BRect rect);
	virtual void	Draw(BView *aView, const BRect rect);
	
	virtual BRect	Frame() ;
	virtual bool	Contains(const BPoint );
	
	// Graphic Movement
	virtual void	MoveTo(const float x, const float y);
	virtual void	MoveBy(const float x, const float y);
		
		void	Invalidate();
		void	Recalculate();
		
		void	SetText(const char *aString);
		void	SetAlignment(const alignment theAlignment);
		void	SetOrigin(const BPoint& origin);
		void	SetFontInfo(const BFont &);
		void	SetFontName(const char *aFontName);
		void	SetShear(const float shear=90);
		void	SetSize(const float fontSize);
		void	SetRotation(const float rotation);
		void	SetHighColor(const rgb_color);
		void	SetLowColor(const rgb_color);
		void	SetColor(const rgb_color);
		void	SetDrawingMode(const drawing_mode mode);
		
		void	GetFontInfo(BFont &info) const;
		
protected:
	BFont		fFontInfo;
	rgb_color	fColor;
	rgb_color	fHighColor;
	rgb_color	fLowColor;
	drawing_mode	fDrawingMode;
	alignment	fAlignment;
	char		*fString;
	BPoint		fStartPoint;
	BPoint		fBasePoint;
	bool		fNeedsCalculation;
	bool		fIsDebugging;
	
private:
};

#endif

