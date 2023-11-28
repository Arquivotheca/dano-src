//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <Window.h>
#include <View.h>
#include <print/PrinterAddOn.h>

#ifndef _PREVIEW_WINDOW_H
#define _PREVIEW_WINDOW_H

class BView;
class BButton;
class BMenuField;
class BMenu;
class BTextControl;
class BPreview;

class PRView : public BView
{
public:
					PRView(BRect frame, BPrinterAddOn& addon, int page);
					~PRView(void);
	virtual	void	AttachedToWindow();
	virtual	void	Draw(BRect frame);
	virtual void	MouseDown(BPoint pt);
	virtual void	MouseUp(BPoint pt);
	virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	virtual void	FrameResized(float w, float h);

			void set_scale(float f);
			float scale() const {return fUserScale;}
			void set_position(const BPoint& p);

			status_t do_page(uint32 page_number);

			int32 Page() const {return page;}
			float ContentWidth();
			float ContentHeight();
			BRect ContentRect();

private:
			void do_image(float scale, BRect rect, uint32 picture_count, BPicture *pPicturesTab[], BPoint *where, BRect *clips);

	BPrinterAddOn& fAddOn;
	float fViewScale;
	float fUserScale;
	BPoint fPosition;
	float fFontHeight;
	BPoint startSrolling;
	bool fTrackingForScrolling;

	// The datas of the current page rendering
	BPrinterAddOn::page_t fPageInfo;
	uint32 picture_count;
	BPoint *Points;
	BRect *Clips;
	BPicture **pPicturesTab;
	uint32 page;
};



class PreviewControlView : public BView
{
public:
			PreviewControlView(const BPoint& position);
	virtual	void AttachedToWindow();
	void SetButtons(bool prev, bool next);
	void SetScaleLabel(const char *label);
	void SetPage(int32 page);
private:
	BButton *fPrev;
	BButton *fNext;
	BMenuField *fScaleMenuField;
	BMenu *fScaleMenu;
	BTextControl *fTcPages;
};


class PreviewWindow : public BWindow
{
public:
		PreviewWindow(BPrinterAddOn& addon);
	virtual ~PreviewWindow();	
	virtual void MessageReceived(BMessage *);
	virtual void FrameResized(float w, float h);
	virtual void Zoom(BPoint origin, float w, float h);

	void Wait();
	BRect best_rect_for_ratio(const BRect& r, const float hw);

private:
	void update_scrollbars();
	void update_buttons();
	void update_menufield();
	void minimal_size(float *w, float *h);

	BPrinterAddOn& fAddOn;
	PRView *fView;
	PreviewControlView *fControlView;
	BScrollView *fScrollView;
	float fFontHeight;
	sem_id fSem;
};

#endif
