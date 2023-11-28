#ifndef _TRIANGLETAB_H_
#define _TRIANGLETAB_H_

#include <Control.h>

class TriangleTab : public BControl
{
public:
	TriangleTab(	BRect frame,
					const char *name,
					const char *label,
					BMessage *message,
					ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					ulong flags = B_WILL_DRAW);
					
	virtual void	MouseDown(BPoint);
	virtual void	Draw(BRect fr);
private:
	void			Animate(bool opening);
};
	
	
#endif
