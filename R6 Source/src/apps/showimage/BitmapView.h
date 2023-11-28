/*	BitmapView.h
 */

#include <View.h>
#include <String.h>

#include <map>

class BBitmap;

class BitmapView :
	public BView
{
public:
								BitmapView(
									BBitmap *		map,
									const char *	comment,
									const BMessage *slices = NULL);
								~BitmapView();

		float					ZoomFactor() const;
		void					SetZoomFactor(
									float			zoom);
		
		void					SetOwnership(
									bool			own);
		bool					GetOwnership();
		void					SetBitmap(
									BBitmap *		map);
		BBitmap *				GetBitmap();
		void					SetAdjustBars(
									bool			adjusts);
		bool					GetAdjustBars();
		BRect					GetMargins();
		void					SetMargins(
									BRect margins);

		void					EnableSlicing(
									bool enabled);
		bool					IsSlicing() const;
		
		void					SetSlices(const BMessage* from);
		void					GetSlices(BMessage* into) const;
		
		void					AddSlice(
									const char*		name,
									BRect			slice,
									bool			unique = false);
		
		void					SetActiveSlice(
									const char *	name);
		const char*				ActiveSlice(
									BRect *			rect = NULL) const;
		
		void					RemoveSlice(
									const char *	name);
		
		virtual	void			AttachedToWindow();
		virtual void			Draw(
									BRect			area);
		virtual void			MouseDown(
									BPoint			where);
		virtual	void			MouseUp(
									BPoint			where);
		virtual	void			MouseMoved(
									BPoint			where,
									uint32			code,
									const BMessage *a_message);
		virtual void			FrameResized(
									float			newWidth,
									float			newHeight);
		virtual void			WindowActivated(
									bool			state);

		void					SetSelection(
									BRect			sel,
									BBitmap *		map = NULL);
		virtual void			Pulse();
		virtual void			MessageReceived(
									BMessage * message);

		bool					HasSelection();
		bool					CanUndo();
		bool					CanPaste();

		void					Undo();
		void					Cut();
		void					Copy();
		void					Paste();
		void					Clear();
		void					SelectAll();

		BRect					RealBitmapBounds() const;

protected:

		float					fZoom;
		
		BRect					fMargins;
		BBitmap *				fBitmap;
		bool					fAdjustBars;
		bool					fOwnership;
		BPoint					fCommentPos;
		char					fComment[256];
		bool					fDrawComment;
		BRect					fSelection;
		pattern					fSelPattern[4];
		int						fClipCount;
static	bool					fTempAnimBreak;
		BBitmap *				fDithered;
		BBitmap *				fFloater;
		BBitmap *				fPromised;
		const BCursor*			fCurCursor;
		
		map<BString,BRect>		fSlices;
		BString					fActiveSlice;
		
		bool					fSlicing;
		
		enum slice_location {
			SLICE_NOTHING,
			SLICE_SELECT,
			SLICE_MOVE,
			SLICE_LEFT,
			SLICE_TOP,
			SLICE_RIGHT,
			SLICE_BOTTOM,
			SLICE_LEFTTOP,
			SLICE_RIGHTTOP,
			SLICE_LEFTBOTTOM,
			SLICE_RIGHTBOTTOM
		};
		
		enum mouse_state {
			RESTING,
			CREATING_FLOATER,
			SELECTING_SLICE,
			MOVING_SLICE,
			CREATING_SLICE
		};
		mouse_state				fMouseState;
		bool					fMouseDown;
		BPoint					fAnchor;
		BPoint					fLastPos;
		BRect					fConstraint;
		BString					fMouseSlice;
		BRect					fOrigSlice;
		slice_location			fMoveLocation;
		
		struct undo_info {
			undo_info(BitmapView * view, bool save_bits = false);
			~undo_info();
			void apply(BitmapView * view);
			BBitmap * floater;
			BRect selection;
			int old_bits;
		};
		undo_info * 			fUndo;

		BRect					BitmapBounds() const;
		void					AdjustBars();
		void					StrokeSelection();
		void					SimpleData(
									BMessage * message);
		void					CopyTarget(
									BMessage * message);
		void					TrashTarget(
									BMessage * message);
		void					DoDrag(
									BMessage * message,
									BPoint delta);
		void					CheckerBoard(
									BBitmap * map,
									BView * view);
		BBitmap *				DitherBitmap(
									BBitmap * from,
									color_space to_space);
		void					DropFloater();
		void					EditMunge(
									bool undo,
									bool clip,
									bool clear);

		void					InvalidateSelection(
									const BRect& r) const;
		
		void					ChooseCursor(
									int32 id);
		
		void					MouseDownSlice(
									BPoint			where);
		
		void					InvalidateSlice(
									const char *	name) const;
		void					SlicesChanged(
									const char *	name = NULL) const;
		
		map<BString,BRect>::const_iterator
								SliceAt(
									BPoint where,
									bool prefer_active=false,
									slice_location *location=NULL) const;
		int32					CursorFromLocation(
									slice_location location) const;
		
	friend struct undo_info;
};
