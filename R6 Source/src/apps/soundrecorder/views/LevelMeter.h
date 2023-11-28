#ifndef LEVEL_METER
#define LEVEL_METER

#include <GraphicsDefs.h>
#include "RenderView.h"

class BBitmap;

class LevelMeter : public RenderView
{
	public:
		LevelMeter( 
			BRect frame, 
			const char *name = "", 
			uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT, 
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS );
		virtual ~LevelMeter( void );
		
		inline void SetLevel( int32 i, float level ) { this->level[i] = level; };
		inline float GetLevel( int32 i ) { return level[i]; };
		
	protected:
		// RenderView Hooks
		virtual void ThreadInit( void );
		virtual status_t ThreadExit( void );
		virtual void Render( BBitmap *bmap );
		virtual void Resize( void );
		
		void InitElements( void );
		void NextLevels( void );
		
		void RenderMeter( 
			BBitmap *bmap, 
			rgb_color color, 
			BRect destRect, 
			float *elements,
			int32 elementCount );
	
	protected:
		float		level[2];
		float		lastLevel[2];
		float		*elementPtr;
		float		*elements[2];
		int32		elementCount;
};

#endif
