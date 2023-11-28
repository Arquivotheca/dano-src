#ifndef GRAPHICELEMENTS_H
#define GRAPHICELEMENTS_H

#include <GraphicsDefs.h>
#include <Point.h>
#include "graphic.h"

class BView;
class BBitmap;
class BGraphPort;
class BGUIStyle;

/*
	Interface: BMGBitmap
	
	Basic graphic that represents a Bitmap in the display.  This class
	can take a BBitmap as a source, or the individual pieces that comprise
	a bitmap.  It will take care of all the movement commands, as well as
	optimized drawing.
*/

class BMGBitmap : public BMGraphic
{
public:
			BMGBitmap();
			BMGBitmap(const char *name, BBitmap *aBitmap=0);
			BMGBitmap(const char *aName,
				const int32 width, const int32 height, 
				color_space cspace, const int8 bytesperpixel, 
				const unsigned char bits[]);
			
	virtual void	Draw(BView *view, const BRect rect);
	virtual void	Draw(BGraphPort *view, const BRect rect);
			
	// Our own interface
			void	SetDrawingMode(const drawing_mode mode);
	virtual void	SetBitmap(BBitmap *);
	
	BBitmap	*	Bitmap() {return fBitmap;};
	
protected:
	BBitmap	*fBitmap;
	drawing_mode	fDrawingMode;
	
private:
};


class BMGRectangle : public BMGraphic
{
public:
			BMGRectangle(const BRect, rgb_color );
			
	virtual void	Draw(BGraphPort *view, const BRect rect);
	virtual void	Draw(BView *view, const BRect rect);

protected:
	BRect	fRect;
	rgb_color	fFillColor;
		
private:
};

class BMGRaisedRect : public BMGraphic
{
public:
			BMGRaisedRect(const BRect, const rgb_color, const int32 border_width = 3, BGUIStyle *aStyle = 0);
			
	virtual void	Draw(BGraphPort *, const BRect rect);

protected:

private:
	BGUIStyle	*fGUIStyle;
	rgb_color	fFillColor;
	int32		fBorderWidth;
};

class BMGSunkenRect : public BMGraphic
{
public:
			BMGSunkenRect(const BRect, const rgb_color, const int32 border_width = 3, BGUIStyle *aStyle = 0);
			
	virtual void	Draw(BGraphPort *, const BRect rect);

protected:

private:
	BGUIStyle	*fGUIStyle;
	rgb_color	fFillColor;
	int32		fBorderWidth;
};

/*
	Interface: BMGSunkenHLine
	
	This graphic will display a sunken horizontal line in the specified
	color.
*/

class BMGSunkenHLine : public BMGraphic
{
public:
			BMGSunkenHLine(const BPoint startPoint, const int32 length, const rgb_color aColor, const int32 line_width=3);
			
	virtual void	Draw(BGraphPort *, const BRect rect);

protected:

private:
	rgb_color	fFillColor;
	int32		fWidth;
	int32		fLength;
};

#endif
