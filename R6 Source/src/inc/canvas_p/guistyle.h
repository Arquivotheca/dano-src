#ifndef GUISTYLE_H
#define GUISTYLE_H

#include <Font.h>

#include "ggraphport.h"
#include "gcolors.h"

enum EGUIStyles {
    Be = 1L,
	Motif,
    Win31,
    Win95,
	OS2,
	UNKNOWN=0xff
};


class BGUIStyle
{
public:
	virtual 		~BGUIStyle();
	
	virtual void	DrawSunkenRect(BGraphPort *, int x, int y, int w, int h, int border_width);
    virtual void	DrawRaisedRect(BGraphPort *, int x, int y, int w, int h, int border_width);
	virtual void	DrawLine(BGraphPort *, int x1, int y1, int x2, int y2, int border_width);

    virtual rgb_color GetSunkenColor() const;
    virtual rgb_color GetRaisedColor() const;
    virtual rgb_color GetBackground() const; 

    virtual int GetBorderWidth() const;
    virtual int GetPadding() const;


    virtual const BFont * GetLabelFont();


    static void 		SetDefaultStyle(BGUIStyle *style);
    static BGUIStyle *	GetDefaultStyle();


protected:
    static BGUIStyle *defaultStyle;

private:
};


void darker(const rgb_color color, rgb_color &another);
void brighter(const rgb_color color, rgb_color &another);

#endif
