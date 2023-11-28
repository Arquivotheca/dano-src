/*
		MenuWindow.h

*/

#ifndef _MENU_WINDOW_H
#define _MENU_WINDOW_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _CLASS_INFO_H
#include <ClassInfo.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

class BMenu;
class BMenuItem;
class BMenuScroller;
class BMenuFrame;
class BMenuWindow;

class BMenuFrame : public BView
{
public:
	BMenuFrame(BMenuWindow *window, bool snakeMenu = false);

	void Draw(BRect, bool clipDrawing, bool clipWindowShape);
	void Clip();

	virtual	void Draw(BRect updateRect);
	virtual void DrawAfterChildren(BRect r);
	virtual	void AttachedToWindow();
	virtual void WindowActivated(bool);
	virtual void FrameResized(float new_width, float new_height);

	BMenuWindow	*fWindow;
	float fEmptyH;
	bool fSnakeMenu;
	float fLastWidth, fLastHeight;
	// the following would be in BMenu, we have them here because
	// BMenu is running out of reserved items and BMenuFrame is heavily
	// involved in the snake rendering anyhow
	BRect fCachedSuperitemRect;
		// for accessing the snake start point
	BRect fLastSnakeSelectionRect;
		// helps with a smart draw during the next selection change
};

class BMenuWindow : public BWindow
{
public:
	BMenuWindow(const char *title = NULL, bool snakeMenu = false);		
	virtual ~BMenuWindow();
		
	void SetMenu(BMenu *menu);
	void AddScrollerViews();
	void RemoveScrollerViews();
	void UpdateScrollers();
	BMenuScroller *UpScroller();
	BMenuScroller *DownScroller();

private:
friend class BWindow;
friend class BMenuFrame;

	BMenu *fMenu;
	BMenuScroller *fUp;
	BMenuScroller *fDown;
	BMenuFrame *fFrame;
};

class BMenuScroller : public BView
{
public:
					BMenuScroller(BRect bounds, bool up);
		void		Enable(bool enable);
virtual	void		Draw(BRect updateRect);
virtual	void		AttachedToWindow();

private:
		bool		fEnabled;
		bool		fUp;
};

inline BMenuScroller *BMenuWindow::UpScroller() { return fUp; };
inline BMenuScroller *BMenuWindow::DownScroller() { return fDown; };

#endif
