// ============================================================
//  CColorMenuItem.h	©1996 Hiroshi Lockheimer
// ============================================================



#include <MenuItem.h>


class CColorMenuItem : public BMenuItem {
public:
					CColorMenuItem(const char *label, rgb_color color,
								   BMessage *message, bool useColor = TRUE,
								   char shortcut = 0, ulong modifiers = 0);

	bool			IsColorEqual(rgb_color color);
	
protected:
	virtual	void	DrawContent();
	
protected:
	rgb_color		mColor;
	bool			mUseColor;
};
