//*****************************************************************************
//
//	File:		 MockupView.h
//
//	Description: Mockup drawing view header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined MONITORVIEW_INCLUDED
#define MONITORVIEW_INCLUDED

#include <View.h>
#include <Rect.h>

class MockupView : public BView
{
public:
	enum	mockup_t
	{
		MOCKUP_MONITOR,
		MOCKUP_FOLDER
	};
			MockupView(const char *name, BView *preview,
				uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_WILL_DRAW);
	void	AttachedToWindow(void);
	void	Draw(BRect upd);
	void	SetMode(mockup_t mode);

private:
	mockup_t	mode;
};

#endif
