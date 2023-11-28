#ifndef _RESIZEVIEW_H_
#define _RESIZEVIEW_H_

#include <View.h>

/*********************************************************/
class ResizeView : public BView
{
public:
	ResizeView(	BRect frame,
				const char *name,
				ulong resizeMask,
				ulong flags);
	
	virtual void	AttachedToWindow();
	virtual void	Show();
	
	void			SizeWindow();
	
	float			winHeight;
};

#endif
