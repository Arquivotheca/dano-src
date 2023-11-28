#include <stdio.h>
#include <Bitmap.h>
#include <Window.h>

#include "RenderView.h"

BRenderView::BRenderView( 
	BRect frame, 
	color_space depth, 
	const char *name, 
	uint32 resizeMask, 
	uint32 flags )
	: 	BView( frame, name, resizeMask, flags | B_WILL_DRAW | B_FRAME_EVENTS ),
		fOffMap( NULL ),
		fSnoozeTime( 40000 ),
		fDepth( depth ),
		fFirstRender( false ),
		fWillSuspend( false )
{
	fBoundsCache = Bounds();
	SetViewColor( B_TRANSPARENT_COLOR );
}
		
BRenderView::~BRenderView( void )
{
	if( fOffMap )
		delete fOffMap;
}

void BRenderView::FrameResized( float new_width, float new_height )
{
	fNewH = new_height;
	fNewW = new_width;
	if(fNewW < 1)
		fNewW = 1;
	if(fNewH < 1 )
		fNewH = 1;
	fUpdateFrame = true;
	Resume();
}

void BRenderView::Draw( BRect updateRect )
{
	// Start render thread if first call to draw
	if( !fFirstRender )
	{
		fFirstRender = true;
		Resume();
	}
	// If the view has not changed, draw using the last rendered bitmap
	else if( fRunSuspended )
		DrawBitmap( fOffMap, updateRect, updateRect );
	else
	{
		fUpdateRect = updateRect;
		fFullUpdate = true;
	}
}

void BRenderView::AttachedToWindow( void )
{
	BitmapInit( &fOffMap, fBoundsCache, fDepth );
	fBoundsCache = Bounds();
	Run();
	BView::AttachedToWindow();
}

void BRenderView::DetachedFromWindow( void )
{
	Quit();
	BView::DetachedFromWindow();
}

status_t BRenderView::Run( int32 priority )
{
	fBoundsCache = Bounds();
	fRunning = true;
	fRunSuspended = false;
	fRenderThread = spawn_thread( render_entry_point, Name(), priority, this );
	if( fRenderThread < 0 )
		return fRenderThread;
	return B_OK;
	// return resume_thread( fRenderThread );
}

status_t BRenderView::Quit( void )
{
	if( fRunning )
	{
		fRunning = false;
		status_t status;
		
		suspend_thread( fRenderThread );
		snooze(1000);
		resume_thread( fRenderThread );
		return wait_for_thread( fRenderThread, &status );
	}
	return B_OK;
}

void BRenderView::SetPriority( int32 priority )
{
	set_thread_priority( fRenderThread, priority );
}

void BRenderView::SetFrameRate( int32 fps )
{
	fSnoozeTime = 1000000/fps;
}

void BRenderView::SetSuspend( bool fWillSuspend )
{
	this->fWillSuspend = fWillSuspend;
}

void BRenderView::Resume( void )
{
	fRunSuspended = false;
	fUpdateNeeded = true;
	resume_thread( fRenderThread );
}

int32 BRenderView::render_entry_point( void *arg )
{
	BRenderView *obj = (BRenderView *)arg;
	return( obj->RenderLoop() );
}

void BRenderView::Resize( void )
{
	fBoundsCache.right = fNewW;
	fBoundsCache.bottom = fNewH;
	BitmapInit( &fOffMap, fBoundsCache, fDepth );
}

void BRenderView::ThreadInit( void )
{

}

status_t BRenderView::ThreadExit( void )
{
	return B_OK;
}

int32 BRenderView::RenderLoop( void )
{
	BWindow		*window = (BWindow *)Looper();
	fUpdateFrame = false;
	fUpdateNeeded = false;
	bigtime_t	startT, deltaT = 0;
	ThreadInit();
	BRect renderRect;
	fFullUpdate = true;
	fUpdateRect = fBoundsCache;
	
	while( fRunning )
	{
		startT = system_time();
		fRunSuspended = false;
		fUpdateNeeded = false;
		
		// Should we resize the bitmap?
		if( fUpdateFrame )
		{
			// Wait until we get a lock or we are instructed to quit
			while( window->LockWithTimeout( 20000 ) != B_OK )
			{
				if( !fRunning )
					goto exit_thread;
			}
			Resize();
			fUpdateFrame = false;
			window->Unlock();
		}
		
		fOffMap->Lock();
		
		// Render to offscreen bitmap
		renderRect = Render();
		if( fFullUpdate )
		{
			renderRect = renderRect | fUpdateRect;
			fFullUpdate = false;
		}
		
		// Wait until we get a lock or we are instructed to quit
		while( window->LockWithTimeout( 20000 ) != B_OK )
		{
			if( !fRunning )
			{
				fOffMap->Unlock();
				goto exit_thread;
			}
		}
		
		// Update window
		DrawBitmap( fOffMap, renderRect, renderRect );
		window->Unlock();
		fOffMap->Unlock();
		
		// Should we suspend?
		if( !fUpdateFrame && !fUpdateNeeded && ShouldSuspend() )
		{
			if( !fRunning )
				goto exit_thread;
			fRunSuspended = true;
			suspend_thread( fRenderThread );
		}
		else
		{
			// Snooze to limit frame rate
			deltaT = system_time() - startT;
			if( deltaT < fSnoozeTime )
				snooze( fSnoozeTime - deltaT );
		}
	}
	
	exit_thread:
	return ThreadExit();
}

bool BRenderView::ShouldSuspend( void )
{
	return fWillSuspend;
}

void BRenderView::BitmapInit( BBitmap **bmap, BRect bounds, color_space depth )
{
	if( *bmap )
		delete *bmap;
	*bmap = new BBitmap( bounds, depth );
}

bool BRenderView::Running( void )
{
	return fRunning;
}

BBitmap *BRenderView::OffscreenBitmap( void )
{
	return fOffMap;
}

BRect BRenderView::BoundsCache( void )
{
	return fBoundsCache;
}

color_space BRenderView::BitDepth( void )
{
	return fDepth;
}

