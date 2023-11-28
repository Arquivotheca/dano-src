#ifndef SLIDE_RULE_H
#define SLIDE_RULE_H

#include <View.h>
#include <Invoker.h>
#include <List.h>
#include "RenderView.h"

class BBitmap;
class BControl;

enum {
	B_TOP_INDICATOR = 1,
	B_BOTTOM_INDICATOR = 2,
	B_RIGHT_INDICATOR = 4,
	B_LEFT_INDICATOR = 8
};

enum {
	B_HORIZONTAL_SLIDERULE = 1,
	B_VERTICAL_SLIDERULE = 2
};

// coordinates referred to as "points" are in the view's coordinate system
// coordinates referred to as "positions" are in the SlideRule's coordinate system
// The Position() is the SlideRule coordinate pointed to by the view's indicator

// Examples...
// Position() = 0, IndicatorCoordinate() = 3
// View Coords:		0	1	2	3	4	5	6
// SlidRule Coords:	-3	-2	-1	0	1	2	3
// Indicator:					^

// Position() = 2, IndicatorCoordinate() = 4
// View Coords:		0	1	2	3	4	5	6
// SlidRule Coords:	-2	-1	0	1	2	3	4
// Indicator:						^

class SlideRule : public BRenderView
{
	public:
		SlideRule(	const char *name,
					BRect frame,
					color_space depth,
					uint32 orientation = B_HORIZONTAL_SLIDERULE,
					uint32 indicators = B_TOP_INDICATOR | B_BOTTOM_INDICATOR,
					uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT,
					uint32 flags = 0 );
		virtual ~SlideRule( void );
		
		// BView Hooks
		virtual void MouseDown( BPoint where );
		virtual void MouseUp( BPoint where );
		virtual void MouseMoved( BPoint where, uint32 code, const BMessage *a_message );
		virtual void SetViewColor( rgb_color color );
		virtual void AttachedToWindow( void );
		virtual	void MakeFocus(bool focusState = true);
		
		void SlideBy( BPoint displacement );
		virtual void SlideTo( BPoint position );
		void SetPosition( BPoint position );
		BPoint Position( void );
		BPoint Desination( void );
		
		// Coord. system translation
		BPoint PositionToPoint( BPoint position );
		BPoint PointToPosition( BPoint where );
		
		void SetIndicatorPosition( uint32 which, float posistion );
		float IndicatorPosition( uint32 which );
		virtual float IndicatorCoordinate( uint32 which );
		BPoint IndicatorPoint( void );
		
		void SetIndicatorMargin( float margin );
		float IndicatorMargin( void );
		
		void SetIndicatorSize( float size );
		float IndicatorSize( void );
		
		void SetOrientation( uint32 orientation );
		uint32 Orientation( void );
		
		uint32 Indicators( void );
		void SetIndicators( uint32 indicators );
		
		void SetWrap( bool wrap );
		bool Wrap( void );
		
		virtual void SetEnabled( bool enabled );
		bool IsEnabled( void );
		
		inline void SetParent( BControl *parent ) { fParent = parent; };
		BView *OffscreenView( void ); // Get the offsecreen view used for drawing
		
	protected:
		virtual void DrawBorder( void );
		virtual void DrawIndicator( uint32 which );
		virtual void DrawContent( void );
		
		virtual void Click( BPoint where ); // Called when clicked
		virtual void Drag( BPoint displacement ); // Called while being dragged
		virtual BPoint Snap( BPoint where ); // Find nearest coordinate to snap to
		virtual void Invoke( void );
		
		// BRenderView Hooks
		virtual void BitmapInit( BBitmap **bmap, BRect bounds, color_space depth );
		virtual bool ShouldSuspend( void ); 
		virtual void Render( void );
	
	private:
		status_t FindIndicator( uint32 which, float **indicator );
	
	protected:
		BControl *fParent;
		
	private:
		BPoint fPosition;
		BPoint fDestination;
		float fLeftInd, fTopInd, fRightInd, fBottomInd;
		float fIndicatorMargin, fIndicatorSize;
		uint32 fOrientation;
		uint32 fIndicators;
		BView *fBMview;
		BPoint fMouse;
		int32 fPressing, fTracking;
	protected:
		rgb_color fVcolorCache;
	private:
		bool fSliding;
		bool fEnabled;
		bool fIsFocus;
		bool fWrap;
};

class IconSlideRule : public SlideRule
{
	public:
		IconSlideRule(	const char *name,
					BRect frame,
					uint32 orientation = B_HORIZONTAL_SLIDERULE,
					uint32 indicators = B_TOP_INDICATOR | B_BOTTOM_INDICATOR,
					uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT,
					uint32 flags = 0,
					color_space depth = B_CMAP8 );
		virtual ~IconSlideRule( void );
		
		virtual void KeyDown( const char *bytes, int32 numBytes );
		
		// App retains reponsibility for deleting icons
		bool AddIcon( const char *label, BBitmap *largeIcon, BBitmap *smallIcon, int32 index=-1 );
		status_t RemoveIcon( int32 icon );
		int32 CountIcons( void );
		
		void SlideToIcon( int32 icon );
		void SlideToNext( void );
		void SlideToPrevious( void );
		
		void SetToIcon( int32 icon );
		int32 CurrentIcon( void );
		
		void SetIconSpacing( float space );
		float IconSpacing( void );
		
		int32 IconForPosition( BPoint position );
		BPoint IconPosition( int32 icon );
		int32 IconForPoint( BPoint where ); // IconForPosition( PositionForPoint(where) )
		BPoint IconPoint( int32 icon ); // PositionToPoint( IconPosition(icon) )
	
	protected:
		virtual void DrawContent( void );
		virtual BPoint Snap( BPoint where );
		virtual void Invoke( void );
		virtual void Click( BPoint where );
		
	private:
		float		fIconSpacing;
		BList		fIconList;
};


#endif
