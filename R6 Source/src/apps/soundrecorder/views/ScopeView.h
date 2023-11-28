#ifndef SCOPE_VIEW_H
#define SCOPE_VIEW_H

#include "RenderView.h"
#include "RecorderWin.h"

class BMediaFile;
struct entry_ref;

class ScopeView : public RenderView
{
	public:
		ScopeView( 	BRect frame, 
			BList *saveFormats = NULL,			
			const char *name = "", 
			uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, 
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS );
		virtual ~ScopeView( void );
		
		virtual void MouseDown( BPoint where );
		void SetTo( entry_ref *ref );
		
		void SetInPoint( float inPoint );
		void SetOutPoint( float outPoint );
		void SetPosition( float position );
		
		inline bigtime_t Duration( void ) { return duration; };
		
		void WaitForClose( void );
	
	protected:
		void RenderScope( BBitmap *bmap, BRect bounds );
		void RenderPosition( BBitmap *bmap, float where );
		bool TransformScopeBounds( void );
		
		void CalculateScopePoints( void );
		
		virtual void Render( BBitmap *bmap );
		virtual void Resize( void );
		virtual bool ShouldSuspend( void );
		virtual void ThreadInit( void );
		virtual status_t ThreadExit( void );
		
	protected:
		BMediaFile		*soundFile;
		entry_ref		*fileRef;

		BList			*saveFormats;
		
		float			inPoint;
		float			outPoint;
		float			position, lastPos;
		
		float			*scopePointsPtr;
		float			*scopePoints[2];
		int32			soundChannels;
		
		thread_id		waitingThread;
		bool			drawScope;
		bool			newFile;
		bool			closeComplete;
		bool			interrupt;
		
		bool			initOK;
		bigtime_t		duration;
		
		BRect			scopeBounds;
		BRect			targetBounds;
};

#endif
