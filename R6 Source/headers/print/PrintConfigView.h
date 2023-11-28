// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_CONFIG_VIEW_H_
#define _PRINT_CONFIG_VIEW_H_

#include <View.h>
#include <Message.h>

namespace BPrivate
{
	class TPrintTools;
	struct _configview_private;
}

class BPrintConfigView : public BView
{
public:
			BPrintConfigView(	BRect frame,
								const char *name,
								uint32 resizeMask,
								uint32 flags);

	virtual ~BPrintConfigView();

	// Archiving
	BPrintConfigView(BMessage *data);
	static	BArchivable *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;

	// Called when the user presses "save"
	virtual status_t Save();
	
	// Allow to enable/disable the window's "save" button
	status_t SetSaveEnabled(bool enable = true);

	// Allow some kind of automatic layout
	virtual void GetMinimumSize(float *width, float *height);

	// Override, for future override compatibility
	virtual	void AttachedToWindow();
	virtual	void AllAttached();
	virtual	void DetachedFromWindow();
	virtual	void AllDetached();
	virtual	void MessageReceived(BMessage *msg);
	virtual	void Draw(BRect updateRect);
	virtual	void MouseDown(BPoint where);
	virtual	void MouseUp(BPoint where);
	virtual	void MouseMoved(BPoint where, uint32 code, const BMessage *a_message);
	virtual	void WindowActivated(bool state);
	virtual	void KeyDown(const char *bytes, int32 numBytes);
	virtual	void KeyUp(const char *bytes, int32 numBytes);
	virtual	void Pulse();
	virtual	void FrameMoved(BPoint new_position);
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void TargetedByScrollView(BScrollView *scroll_view);
	virtual	void SetFlags(uint32 flags);
	virtual	void SetResizingMode(uint32 mode);
	virtual	void MakeFocus(bool focusState = true);
	virtual	void Show();
	virtual	void Hide();
	virtual	void GetPreferredSize(float *width, float *height);
	virtual	void ResizeToPreferred();
	virtual BHandler *ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property);
	virtual status_t GetSupportedSuites(BMessage *data);
	virtual status_t Perform(perform_code d, void *arg);
	virtual	void DrawAfterChildren(BRect r);	

private:
	friend class BPrintJob;
	friend class BPrivate::TPrintTools;

	virtual status_t _Reserved_BPrintConfigView_0(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_1(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_2(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_3(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_4(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_5(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_6(int32 arg, ...);
	virtual status_t _Reserved_BPrintConfigView_7(int32 arg, ...);
	
	BPrivate::_configview_private *fPrivate;
	BPrivate::_configview_private& _m_rprivate;
	uint32 reserved[4];
};

#endif

