#ifndef _ICONMENUITEM_H
#define _ICONMENUITEM_H

// IconMenuItem.h

class IconMenuItem : public BMenuItem
{
public:
	IconMenuItem(const char *label,
				BBitmap *icon,
				BMessage *message);
	IconMenuItem(BBitmap *icon,
				BMenu *submenu);
virtual void	GetContentSize(float *width, float *height);
virtual void	DrawContent();
		void	SetIcon(BBitmap *icon);
virtual void	Highlight(bool high);
	BBitmap		*GetIcon();

private:
	BBitmap *iconBmap;
	bool	space;
};

#endif
