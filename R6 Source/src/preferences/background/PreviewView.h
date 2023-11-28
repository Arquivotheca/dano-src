//*****************************************************************************
//
//	File:		 PreviewView.h
//
//	Description: Preview view header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined PREVIEWVIEW_INCLUDED
#define PREVIEWVIEW_INCLUDED

#include <View.h>
#include <Rect.h>
#include <Invoker.h>
#include <TranslationDefs.h>

class BBitmap;
class BFile;

class PreviewView : public BView, public BInvoker
{
	thread_id	loader;
	float		scale;

	volatile bool	terminate;
	translator_info	info;
	BFile			*loadfile;
	BRect			bigrect;

	// SetViewBitmap arguments
	BBitmap	*bitmap;
	BBitmap	*bigbitmap;

	BPoint		offset;
	int32		mode;
	bool		aspectconstrain;
	BMessenger	notificmessenger;
	BMessage	*notificmessage;

	int32	Loader();
	static int32	LoaderStub(void *data) { return ((PreviewView *)data)->Loader(); }

public:
			PreviewView(BRect frame, const char *name, float ratio, BMessage *badload,
				uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_WILL_DRAW);
			~PreviewView();
	void	DetachedFromWindow();
	void	MessageReceived(BMessage *msg);
	void	MouseMoved(BPoint /*point*/, uint32 transit, const BMessage * /*message*/);
	void	MouseDown(BPoint p);

	void	SetLoadNotification(BMessenger m, BMessage *);
	void	SetAspectConstrain(bool constrain);
	void	SetScale(float ratio);
	bool	LoadBitmap(const char *path);
	void	ClearBitmap();

	void	Manual(float x, float y) { offset.x = x; offset.y = y; mode = 0; }
	void	Center() { mode = 1; }
	void	ScaleToFit() { mode = 2; }
	void	Tile() { mode = 3; }

	void	Redisplay();

	virtual void Move(BPoint p);
};

#endif
