#ifndef RENDER_VIEW_H
#define RENDER_VIEW_H

#include <GraphicsDefs.h>
#include <View.h>
class BBitmap;

class BRenderView : public BView
{
	public:
		BRenderView( 
			BRect frame, 
			color_space depth,
			const char *name = "", 
			uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, 
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS );
		
		virtual ~BRenderView( void );
		
		// BView Hooks
		virtual void FrameResized( float new_width, float new_height );
		virtual void Draw( BRect updateRect );
		virtual void AttachedToWindow( void ); // Calls Run()
		virtual void DetachedFromWindow( void ); // Calls Quit()
		
		status_t Run( int32 priority = B_NORMAL_PRIORITY );
		status_t Quit( void );
		
		void SetPriority( int32 priority ); // default is B_NORMAL_PRIORITY
		void SetFrameRate( int32 fps ); // default is 25 fps
		
		virtual void SetSuspend( bool willSuspend ); // Will suspend when frame complete
		virtual void Resume( void ); // Resume rendering
		
	protected:
		// ****
		// RenderView hooks
		// ****
		virtual void Resize( void );
		virtual void BitmapInit( BBitmap **bmap, BRect bounds, color_space depth );
		
		virtual void ThreadInit( void );
		virtual status_t ThreadExit( void );
		
		virtual bool ShouldSuspend( void ); 
		virtual BRect Render( void ) = 0;
		
		BBitmap *OffscreenBitmap( void );
		color_space BitDepth( void );
		
		BRect BoundsCache( void );
		bool Running( void );
	
	private:
		static int32 render_entry_point( void *arg );
		int32 RenderLoop( void );
		
	private:
		float			fNewH, fNewW;
		BRect 			fBoundsCache; // Cached BView::Bounds()
		BRect			fUpdateRect;
		BBitmap			*fOffMap;
		
		// Thread Stuff
		thread_id		fRenderThread;
		bigtime_t		fSnoozeTime;
		
		color_space 	fDepth;
		// Control Flags
		bool			fFirstRender;
		bool			fRunning; // True while the thread should continue running
		bool			fUpdateFrame; // The frame has been resized and the bitmap should be resized at the next opportunity
		bool			fRunSuspended; // Draw() should perform update when true; else the render thread will take care of the update
		bool			fWillSuspend; // Will suspend after finishing one frame
		bool			fUpdateNeeded;
		bool			fFullUpdate;
};

#endif
