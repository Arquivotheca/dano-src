#ifndef RENDER_VIEW_H
#define RENDER_VIEW_H

#include <GraphicsDefs.h>
#include <View.h>

class BBitmap;

class RenderView : public BView
{
	public:
		RenderView( 
			BRect frame, 
			color_space depth,
			const char *name = "", 
			uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, 
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS );
		
		virtual ~RenderView( void );
		
		// BView Hooks
		virtual void FrameResized( float new_width, float new_height );
		virtual void Draw( BRect updateRect );
		virtual void AttachedToWindow( void ); // Calls Run()
		virtual void DetachedFromWindow( void ); // Calls Quit()
		
		status_t Run( int32 priority = B_NORMAL_PRIORITY );
		status_t Quit( void );
		
		void SetPriority( int32 priority ); // default is B_NORMAL_PRIORITY
		void SetFrameRate( int32 fps ); // default is 25 fps
		
		virtual void Suspend( void ); // Suspend rendering
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
		virtual void Render( BBitmap *bmap ) = 0;
		
		bool Running( void );
	
	private:
		static int32 render_entry_point( void *arg );
		int32 RenderLoop( void );
	
	protected:
		BRect bounds; // Cached BView::Bounds()
		
	private:
		BBitmap			*offMap;
		color_space 	depth;
		bool			running;
		bool			updateFrame;
		bool			runSuspended;
		thread_id		renderThread;
		int32			priority;
		bigtime_t		snoozeTime;
		float			newH, newW;
};

#endif
