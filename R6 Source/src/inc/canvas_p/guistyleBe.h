#ifndef GUISTYLEWIN95_H
#define GUISTYLEWIN95_H

#include "guistyle.h"



class BGUIStyleBe : public BGUIStyle
{
public:
					BGUIStyleBe();
	virtual			~BGUIStyleBe();
					
	virtual void	DrawSunkenRect(BGraphPort *, int x, int y, int w, int h, int border_width);
    virtual void	DrawRaisedRect(BGraphPort *, int x, int y, int w, int h, int border_width);
	virtual void	DrawLine(BGraphPort *, int x1, int y1, int x2, int y2, int border_width);

    virtual rgb_color GetSunkenColor() const;
    virtual rgb_color GetRaisedColor() const;
    virtual rgb_color GetBackground() const; 

    virtual int GetBorderWidth() const;
    virtual int GetPadding() const;

protected:
	rgb_color	background;
	rgb_color	foreground;
	rgb_color	bottomShadow;
	rgb_color	topShadow;
	rgb_color	bottomShadowTopLiner;
	rgb_color	bottomShadowBottomLiner;
	
private:
};

#endif
