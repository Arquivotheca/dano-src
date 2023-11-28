// ============================================================
//  CFontMenuItem.h	©1996 Hiroshi Lockheimer
// ============================================================



#include <MenuItem.h>


class CFontMenuItem : public BMenuItem {
public:
					CFontMenuItem(const char *label, BMessage *message,
								  char shortcut = 0, ulong modifiers = 0);

protected:
	virtual	void	GetContentSize(float *width, float *height);
	virtual	void	DrawContent();
};
