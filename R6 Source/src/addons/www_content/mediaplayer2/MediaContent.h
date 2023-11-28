#ifndef __MEDIA_CONTENT_H__
#define __MEDIA_CONTENT_H__

#include <Content.h>
#include <Handler.h>
#include <Binder.h>
#include "GHandler.h"

using namespace Wagner;


enum widget_id {
	kClosebox,
	kIntersect,
	kPause,
	kPauseActive,
	kPauseOver,
	kPlay,
	kPlayActive,
	kPlayOver,
	kProgressBar,
	kRemainBar,
	kEndcapLeft,		// Note: drawing order is important. Leave these after bar.
	kEndcapRight,

	kWidgetCount
};

class PlaybackEngine;

class MediaContentInstance : public ContentInstance, public BinderNode {
public:
	MediaContentInstance(Content *content, GHandler *handler, const BMessage &msg);
	~MediaContentInstance();
	virtual status_t Draw(BView *into, BRect exposed);
	virtual	status_t GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
	virtual	status_t AttachedToView(BView*, uint32 *contentFlags);
	virtual	status_t DetachedFromView();
	virtual	status_t FrameChanged(BRect newRect, int32 fullWidth, int32 fullHeight);
	virtual	void MouseDown(BPoint where, const BMessage *event=NULL);
	void UpdateProgress();
	
protected:
	virtual	status_t OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t CloseProperties(void *cookie);
	virtual	put_status_t WriteProperty(const char *name, const property &prop);
	virtual	get_status_t ReadProperty(const char *name, property &prop, const property_list
		&args = empty_arg_list);
	virtual	status_t HandleMessage(BMessage*);
	virtual void Notification(BMessage*);
	virtual void Cleanup();

private:
	void Layout();
	static void	PostStopMessage(void*);
	static void PostErrorMessage(void *castToInstance, const char *error);
	static void UpdateProperty(void *castToInstance, const char *prop);
	static void	UpdateNotify(void*);
	void LoadBitmap(widget_id, const char*);

	ContentInstance *fWidgets[kWidgetCount];
	ContentInstance *fMediaBarHTML;
	BRect fWidgetRect[kWidgetCount];
	BRect fContentRect;
	BView *fParentView;
	PlaybackEngine *fPlaybackEngine;
	float fOldDivider;
	bool fPaused;
	BString fTitle;
	BPoint fTitlePos;
	BFont fTitleFont;
	bool fGotTitle;
	bool fGotControlSizes;
	bool fShowControls;
	bool fVisible;
	bool fShowEjectButton;
	bool fAutoStart;
	long fUpdateRunning;

	// Layout parameters
	float fBorder;
	float fPlayButtonWidth;
	float fTitleWidth;
	float fCloseButtonWidth;
	
	rgb_color fBackgroundColor;
	bool fPaintBackground;
	rgb_color fTitleColor;
	float fPreferredWidth;
	float fPreferredHeight;
	const char *fCurrentErrorString;
};

class MediaContent : public Content {
public:
	MediaContent(void *handle);
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t GetMemoryUsage();
	void DrawBitmap(BView *into, widget_id, BRect);

private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler*, const BMessage&);
};

#endif
