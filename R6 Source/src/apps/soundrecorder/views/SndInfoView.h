#ifndef SND_INFO_VIEW_H
#define SND_INFO_VIEW_H

#include <Box.h>
#include <String.h>
#include <Entry.h>

class BSoundFile;

class SndFInfoView : public BBox
{
	public:
		SndFInfoView( 	BPoint 		where, 
						const char 	*name = NULL, 
						uint32 		resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
						uint32 		flags = B_WILL_DRAW | B_FRAME_EVENTS );
		virtual ~SndFInfoView( void );
		
		status_t SetTo( entry_ref *ref );
		void Unset( void );
		
		virtual	void Draw( BRect bounds );
		
	protected:
		BSoundFile		*sndFile;
		
		BString			fileNameS;
		BString			formatS;
		BString			compressionS;
		BString			channelsS;
		BString			sampleSizeS;
		BString			sampleRateS;
		BString			lengthS;
};

#endif
