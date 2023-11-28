#ifndef _HELPWINDOW_H_
#define _HELPWINDOW_H_

#include <Window.h>
#include <View.h>
#include <Point.h>
#include <Message.h>
#include <Rect.h>
#include <TextView.h>


// HelpWindow.h

class HelpWindow : public BWindow
{
public:
				HelpWindow(const char *title,
						   BPoint where,
						   const char *helpText);
virtual void	MessageReceived(BMessage *);

};


class HelpView : public BView
{
public:
				HelpView(BRect frame,
						 const char *_helpText);
virtual void	AttachedToWindow();
virtual void	Draw(BRect up);

private:
		enum {
			BOTTOM_MARGIN = 44,
			BUTTON_WIDTH = 90,
			BUTTON_HEIGHT = 22
		};
		const char *helpText;
};

class HelpTextView : public BTextView
{
public:
			HelpTextView(	BRect frame,
							const char *name,
							const char *theText,
							BRect textRect, 
							ulong resizeMask,
							ulong flags);
							
virtual void	AttachedToWindow();
virtual void	FrameResized(float w, float h);

private:
	const char *helpText;
};


#endif
