#ifndef _ICONMENUITEM_H_
#define _ICONMENUITEM_H_

#include <MenuItem.h>
#include <Bitmap.h>
#include <Message.h>

// IconMenuItem.h

class IconMenuItem : public BMenuItem
{
public:
	IconMenuItem(const char *label,
				BBitmap *icon,
				BMessage *message,
				bool _freeIcon = TRUE);
	IconMenuItem(BBitmap *icon,
				BMenu *submenu);
virtual			~IconMenuItem();
virtual void	GetContentSize(float *width, float *height);
virtual void	DrawContent();
		void	MyInvoke();			// should have invoke public!
		void	SetIcon(BBitmap *icon);
virtual void	Highlight(bool high);
	BBitmap		*GetIcon();

private:
	BBitmap *iconBmap;
	bool	freeIcon;
	bool	space;
};

#endif
