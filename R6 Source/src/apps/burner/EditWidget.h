//
// EditWidget.h
//

#ifndef _EDIT_WIDGET_H_
#define _EDIT_WIDGET_H_

#include <View.h>

class CDTrack;
class BurnerWindow;
class TrackEditView;
class TrackListView;

class EditWidget : public BView
{
public:
					EditWidget(BRect frame, uint32 resizingMode);
	virtual			~EditWidget();

	void			SetTrackList(CDTrack *track);
	CDTrack			*TrackList();
	void			SetSelectedTrack(CDTrack *track, bool selectInList = false);
	
	void			SetEnabled(bool enabled);
	bool			IsEnabled();
	
	void			SetMaxFrames(int32 maxFrames);
	int32			MaxFrames();
	
	void			InvalidateBuffer();
		
protected:
	virtual void	FrameResized(float width, float height);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage *msg);

	virtual void	MouseMoved(BPoint where, uint32 transit, const BMessage *dragMessage);
	virtual void	MouseDown(BPoint where);
	virtual void	MouseUp(BPoint where);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	virtual void	MakeFocus(bool focused = true);
	
	virtual void	Draw(BRect update);

	void			DrawBuffer();
	void			DrawTopBar();
	void			DrawBottomBar();
	void			DrawBrackets();
	void			DrawFaders();
	
	CDTrack			*FindTrackAtPoint(BPoint point) const;
	// constants identifying the various controls in the view
	enum control_type {
		NONE			= 0,
		LEFT_BRACKET	= 1,
		RIGHT_BRACKET	= 2,
		LEFT_FADER		= 3,
		RIGHT_FADER		= 4,
		PREGAP_POINTER	= 5,
		SELECTED_TRACK	= 6
	};
private:
	void			SetBackgroundColor();
	void			CalculateRects(BRect bounds);

	BRect			LeftBracketRect();
	BRect			RightBracketRect();
	BRect			LeftFaderRect();
	BRect			RightFaderRect();
	
	BRect			fTopBarRect;
	BRect			fBottomBarRect;
	BRect			fSelectedTrackRect; // rect of selected track in top bar
	BRect			fUsedRect;			// rect of used data in bottom bar

	BRect			fPregapPointerRect;
	BRect			fBounds;
	
	BPoint			fClickPoint;	// the mouse down point
	BPoint			fClickOffset;	// offset from the top left of the clicked control
	BPoint			fLastPoint;		// the last point where the cursor was tracked
	
	CDTrack			*fHeadTrack;
	CDTrack			*fSelectedTrack;
	BurnerWindow	*fWindow;
	TrackEditView	*fTrackEditView;
	TrackListView	*fTrackListView;
	int32			fPlayingTrack;	// the index of the playing track, 0 if not playing
	int32			fPlayingOffset;	// the byte offset in the playing track
	int32			fMaxFrames;		// the number of frames available on the CD
	control_type	fMovingControl;
	control_type	fFocusedControl;

	bool			fValueChanged;	// whether or not the value has changed during the mouse down
	bool			fEnabled;
	bool			fEditEnabled;
	bool			fBufferValid;
};

#endif // _EDIT_WIDGET_H_
