#ifndef _VIDEO_VIEW_H
#define _VIDEO_VIEW_H

#include <View.h>
#include <TranslationDefs.h>

class VideoView : public BView {
public:

	VideoView(BRect, BPoint videoSize, const char *name, uint32 resizingMode);
	virtual ~VideoView();
	void SetBitmap(BBitmap *, bool isOverlay = false);
	static BRect ViewRect(BRect videoRect);
	BRect ContentRect();
	void SetProportionalResize(bool);
	bool ProportionalResize() const;
	void SetVideoSize(BPoint);

protected:

	virtual void FrameResized(float, float);
	virtual void Draw(BRect);
	virtual void AttachedToWindow();

	static BRect VideoRect(BRect frame, BPoint videoSize);

	virtual void MouseDown(BPoint);
	virtual void MouseMoved(BPoint, uint32 code, const BMessage*);
	virtual void MouseUp(BPoint);
	virtual void MessageReceived(BMessage*);
	bool FindTranslator(const char *mime, translator_id *out_translator, uint32 *out_format);

	// These are used when dragging bitmap clips out of the video view
	void CopyBitmap(BBitmap *src, BBitmap *dest);
	void CopyBitmapYCbCr(BBitmap *src, BBitmap *dest);

private:


	// Display state
	BBitmap	*fDisplayBitmap;
	BWindow	*fWindow;
	BPoint fVideoSize;
	bool fProportionalResize;

	// Overlay info
	bool fUsingOverlay;
	rgb_color fOverlayKeyColor;

	// State for dragging frame clippings
	BPoint fClickPoint;
	bool fBeginDrag;
	BBitmap *fCapturedFrame;

	typedef	BView _inherited;
};

const float kMinViewWidth = 180;
const float kMinViewHeight = 125;

#endif
