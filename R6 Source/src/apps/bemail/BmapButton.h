#ifndef BMAP_BUTTON_H
#define BMAP_BUTTON_H

#include <View.h>
#include <Control.h>
#include <List.h>
#include <Locker.h>

class BBitmap;
class BResources;

class BmapButton : public BControl
{
	public:
		BmapButton( 	BRect frame, 
						const char *name, 
						const char *label,
						int32 enabledID,
						int32 disabledID,
						int32 rollID,
						int32 pressedID,
						bool showLabel,
						BMessage *message,
						uint32 resizeMask,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE  );
		virtual ~BmapButton( void );
		
		// Hooks
		virtual void Draw( BRect updateRect );
		virtual void GetPreferredSize( float *width, float *height );
		virtual void MouseMoved( BPoint where, uint32 code, const BMessage *msg );
		virtual void MouseDown( BPoint point );
		virtual void MouseUp( BPoint where );
		virtual void WindowActivated( bool active );
		
		void InvokeOnButton( uint32 button );
		void ShowLabel( bool show );
	
	protected:
		const BBitmap *RetrieveBitmap( int32 id );
		status_t ReleaseBitmap( const BBitmap *bm );
		
	protected:
		const BBitmap	*fEnabledBM, *fDisabledBM, *fRollBM, *fPressedBM;
		
		int32 		fPressing;
		int32 		fIsInBounds;
		uint32 		fButtons;
		bool 		fShowLabel;
		bool		fActive;
		BRect		fBitmapRect;
		BPoint		fWhere;
		uint32		fIButtons;
	
	private:
		static BList		fBitmapCache;
		static BLocker		fBmCacheLock;
};

#endif
