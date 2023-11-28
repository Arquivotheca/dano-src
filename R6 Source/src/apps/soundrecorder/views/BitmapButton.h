#ifndef BITMAP_BUTTON_H
#define BITMAP_BUTTON_H

#include <Button.h>

class BBitmap;

class BitmapButton : public BButton
{
	public:
		BitmapButton( 
			BPoint where, 
			const char *name, 
			const void *bitmap,
			int32 width,
			int32 height,
			BMessage *message,
			uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE );
		~BitmapButton( void );
		
		virtual void Draw( BRect updateRect );
		virtual void GetPreferredSize( float *width, float *height );
		virtual void AttachedToWindow( void );
	
	protected:
		BBitmap		*normalBM, *highlightBM, *disabledBM;
		int32		height, width, size;
		BRect		bounds;
};

class BitmapCheckBox : public BControl
{
	public:
		BitmapCheckBox( 
			BPoint where, 
			const char *name, 
			const void *bitmap,
			int32 width,
			int32 height,
			BMessage *message,
			uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE );
		~BitmapCheckBox( void );
		
		virtual	void MouseMoved( BPoint where, uint32 code, const BMessage *msg );
		virtual void Draw( BRect );
		virtual void MouseDown( BPoint );
		virtual	void MouseUp( BPoint where );
		virtual void KeyDown( const char *bytes, int32 numBytes );
		virtual void GetPreferredSize( float *width, float *height );
		virtual void AttachedToWindow( void );
		virtual void SetValue( int32 value );
	
	protected:
		enum State {
				kNormal,
				kHighlight,
				kChecked,
				kHighlightChecked,
		};
		
		virtual void DrawInState(BitmapCheckBox::State state);
		virtual State StateForPosition(BPoint mousePos);
		
		BBitmap		*normalBM, *highlightBM, *checkedBM, *highlightCheckedBM, *disabledBM, *disabledCheckedBM;
		int32		height, width, size;
		BRect		bounds;
		
		bool fPressing;
		State fStateWhenPressed, fCurrState;
		BPoint fClickPoint;
};

#endif
