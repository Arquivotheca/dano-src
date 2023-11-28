#include "ButtonBar.h"
#include <stdlib.h>
#include <math.h>

struct BBDivider
{
	float 		where;
	float		vmargin;
	BmapButton	*button;
};

static const int32 kDividerBlockSize = 8;

ButtonBar::ButtonBar( BRect frame, const char *name, uint8 enabledOffset, uint8 disabledOffset, 
uint8 rollOffset, uint8 pressedOffset, float Hmargin, float Vmargin, 
uint32 resizeMask, int32 flags, border_style border )
	: BBox( frame, name, resizeMask, flags, border ),
	fEnabledOffset( enabledOffset ), 
	fDisabledOffset( disabledOffset ), 
	fRollOffset( rollOffset ), 
	fPressedOffset( pressedOffset ), 
	fHMargin( Hmargin ), 
	fVMargin( Vmargin ),
	fMaxHeight( 0 ),
	fMaxWidth( 0 ),
	fNextXOffset( Hmargin ),
	fDividers( 0 ),
	fDividerArray( NULL ),
	fShowLabels( true )
{
	
}

ButtonBar::~ButtonBar( void )
{
	if( fDividerArray )
		free( fDividerArray );
}
						
BmapButton *ButtonBar::AddButton( const char *label, int32 baseID, BMessage *msg )
{
	BmapButton *button;
	
	button = new BmapButton( BRect( 0, 0, 31, 31), label, label, baseID+fEnabledOffset, 
		baseID+fDisabledOffset, baseID+fRollOffset, baseID+fPressedOffset, 
		fShowLabels, msg, B_FOLLOW_LEFT | B_FOLLOW_TOP );
	
	fButtonList.AddItem( button );
	AddChild( button );
	return button;
}

void ButtonBar::Arrange( bool fixedWidth )
{
	// Reset Positioning Info
	fNextXOffset = fHMargin;
	fMaxHeight = 0;
	fMaxWidth = 0;
	
	float width, height;
	BmapButton *button;
	
	// Determine Largest button dimensions
	for( int32 i=0; (button=(BmapButton *)fButtonList.ItemAt(i)); i++ )
	{
		button->GetPreferredSize( &width, &height );
		if( height > fMaxHeight )
			fMaxHeight = height;
		if( width > fMaxWidth )
			fMaxWidth = width;
	}
	
	// Arrange buttons
	for( int32 i=0; (button=(BmapButton *)fButtonList.ItemAt(i)); i++ )
	{
		button->MoveTo( fNextXOffset, fVMargin );
		if( fixedWidth )
		{
			button->ResizeTo( fMaxWidth, fMaxHeight );
			fNextXOffset += fMaxWidth+fHMargin;
		}
		else
		{
			button->GetPreferredSize( &width, &height );
			button->ResizeTo( width, fMaxHeight );
			fNextXOffset += width+fHMargin;
		}
	}
	
	// Move dividers to match
	for( int32 i=0; i<fDividers; i++ )
	{
		if( fDividerArray[i].button )
			fDividerArray[i].where = fDividerArray[i].button->Frame().right + floor(fHMargin/2);
		else
			fDividerArray[i].where = floor(fHMargin/2);
	}
}

void ButtonBar::GetPreferredSize( float *width, float *height )
{
	*width = fNextXOffset+fHMargin;
	*height = fMaxHeight+(2*fVMargin);
}

void ButtonBar::AttachedToWindow( void )
{
	if ( Parent() )
		SetViewColor( Parent()->ViewColor() );
	BBox::AttachedToWindow(); 
}

void ButtonBar::Draw( BRect updateRect )
{
	BBox::Draw( updateRect );
	rgb_color	high = { 184, 184, 184 };
	rgb_color	low = { 232, 232, 232 };
	BRect		bounds = Bounds();
	float		where, vmargin;
	
	BeginLineArray( fDividers*2 );
	for( int32 i=0; i<fDividers; i++ )
	{
		where = fDividerArray[i].where;
		vmargin = fDividerArray[i].vmargin;
		AddLine( BPoint( where, fVMargin+vmargin ), BPoint( where, bounds.bottom-fVMargin-vmargin ), high );
		AddLine( BPoint( where+1, fVMargin+vmargin ), BPoint( where+1, bounds.bottom-fVMargin-vmargin ), low );
	}
	EndLineArray();
}

void ButtonBar::AddDivider( float vmargin )
{
	// Do we need to allocate memory?
	if( fDividers == 0 )
		fDividerArray = (BBDivider *)malloc( sizeof(BBDivider)*kDividerBlockSize );
	if( (fDividers % kDividerBlockSize) == 0 )
		fDividerArray = (BBDivider *)realloc( fDividerArray, sizeof(BBDivider)*kDividerBlockSize*((fDividers/kDividerBlockSize)+1) );
	
	// Cache the location and the button which proceeds it
	// The button is stored because we may later wish to change the layout
	fDividerArray[fDividers].vmargin = vmargin;
	fDividerArray[fDividers].where = fNextXOffset+floor(fHMargin/2);
	fDividerArray[fDividers].button = (BmapButton *)fButtonList.ItemAt(fButtonList.CountItems()-1);
	fDividers++;
}

void ButtonBar::ShowLabels( bool show )
{
	BmapButton *button;
	
	// Set show label flags on buttons
	for( int32 i=0; (button=(BmapButton *)fButtonList.ItemAt(i)); i++ )
		button->ShowLabel( show );
	fShowLabels = show;
}
