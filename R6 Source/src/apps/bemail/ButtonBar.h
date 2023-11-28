#ifndef BUTTON_BAR_H
#define BUTTON_BAR_H

#include "BmapButton.h"
#include <Box.h>

struct BBDivider;

class ButtonBar : public BBox
{
	public:
		ButtonBar( 		BRect frame,
						const char *name,
						uint8 enabledOffset,
						uint8 disabledOffset,
						uint8 rollOffset,
						uint8 pressedOffset,
						float Hmargin, 
						float Vmargin, 
						uint32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
						int32 flags = B_NAVIGABLE_JUMP | B_FRAME_EVENTS | B_WILL_DRAW,
						border_style border = B_FANCY_BORDER );
		virtual ~ButtonBar( void );
		
		// Hooks
		virtual void GetPreferredSize( float *width, float *height );
		virtual void AttachedToWindow( void );
		virtual void Draw( BRect updateRect );
		
		void ShowLabels( bool show );
		void Arrange( bool fixedWidth = true );
		BmapButton *AddButton( const char *label, int32 baseID, BMessage *msg );
		void AddDivider( float vmargin );
		
	protected:
		float			fMaxHeight, fMaxWidth;
		float			fNextXOffset;
		float			fHMargin, fVMargin;
		uint8 			fEnabledOffset, fDisabledOffset, fRollOffset, fPressedOffset;
		BList			fButtonList;
		BBDivider		*fDividerArray;
		int32			fDividers;
		bool			fShowLabels;
};

#endif
