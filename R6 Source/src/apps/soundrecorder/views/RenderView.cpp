#include <stdio.h>
#include <Bitmap.h>
#include <Window.h>

#include "RenderView.h"

RenderView::RenderView( 
	BRect frame, 
	color_space depth, 
	const char *name, 
	uint32 resizeMask, 
	uint32 flags )
	: 	BView( frame, name, resizeMask, flags ),
		depth( depth ),
		snoozeTime( 40000 )
{
	bounds = Bounds();
	offMap = NULL;
	BitmapInit( &offMap, bounds, depth );
}
		
RenderView::~RenderView( void )
{
	if( offMap )
		delete offMap;
}

void RenderView::FrameResized( float new_width, float new_height )
{
	newH = new_height;
	newW = new_width;
	updateFrame = true;
	Resume();
}

void RenderView::Draw( BRect updateRect )
{
	if( runSuspended )
		DrawBitmap( offMap, bounds );
}

void RenderView::AttachedToWindow( void )
{
	bounds = Bounds();
	Run();
	BView::AttachedToWindow();
}

void RenderView::DetachedFromWindow( void )
{
	Quit();
	BView::DetachedFromWindow();
}

status_t RenderView::Run( int32 priority )
{
	bounds = Bounds();
	running = true;
	runSuspended = false;
	renderThread = spawn_thread( render_entry_point, Name(), priority, this );
	if( renderThread < 0 )
		return renderThread;
	return resume_thread( renderThread );
}

status_t RenderView::Quit( void )
{
	if( running )
	{
		running = false;
		status_t status;
		
		suspend_thread( renderThread );
		snooze(1000);
		resume_thread( renderThread );
		return wait_for_thread( renderThread, &status );
	}
	return B_OK;
}

void RenderView::SetPriority( int32 priority )
{
	set_thread_priority( renderThread, priority );
}

void RenderView::SetFrameRate( int32 fps )
{
	snoozeTime = 1000000/fps;
}

void RenderView::Suspend( void )
{
	runSuspended = true;
	suspend_thread( renderThread );
}

void RenderView::Resume( void )
{
	runSuspended = false;
	resume_thread( renderThread );
}

int32 RenderView::render_entry_point( void *arg )
{
	RenderView *obj = (RenderView *)arg;
	return( obj->RenderLoop() );
}

void RenderView::Resize( void )
{
	bounds.right = newW;
	bounds.bottom = newH;
	BitmapInit( &offMap, bounds, depth );
}

void RenderView::ThreadInit( void )
{

}

status_t RenderView::ThreadExit( void )
{
	return B_OK;
}

int32 RenderView::RenderLoop( void )
{
	BWindow		*window = (BWindow *)Looper();
	updateFrame = false;
	bigtime_t	startT, deltaT = 0;
	
	ThreadInit();
	
	while( running )
	{
		startT = system_time();
		runSuspended = false;
		
		// Should we resize the bitmap?
		if( updateFrame )
		{
			// Wait until we get a lock or we are instructed to quit
			while( window->LockWithTimeout( 20000 ) != B_OK )
			{
				if( !running )
					goto exit_thread;
			}
			Resize();
			updateFrame = false;
			window->Unlock();
		}
		
		// Render to offscreen bitmap
		Render( offMap );
		
		// Wait until we get a lock or we are instructed to quit
		while( window->LockWithTimeout( 20000 ) != B_OK )
		{
			if( !running )
				goto exit_thread;
		}
		
		// Update window
		DrawBitmap( offMap, bounds );
		window->Unlock();
		
		// Should we suspend?
		if( ShouldSuspend() )
			Suspend();
		else
		{
			// Snooze to limit frame rate
			deltaT = system_time() - startT;
			if( deltaT < snoozeTime )
				snooze( snoozeTime - deltaT );
		}
	}
	
	exit_thread:
	return ThreadExit();
}

bool RenderView::ShouldSuspend( void )
{
	return false;
}

void RenderView::BitmapInit( BBitmap **bmap, BRect bounds, color_space depth )
{
	if( *bmap )
		delete *bmap;
	*bmap = new BBitmap( bounds, depth );
}

bool RenderView::Running( void )
{
	return running;
}
