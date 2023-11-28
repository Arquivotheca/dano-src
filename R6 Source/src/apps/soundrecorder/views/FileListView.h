#ifndef FILE_LIST_VIEW_H
#define FILE_LIST_VIEW_H

#include <ListView.h>
#include <ListItem.h>
#include <Entry.h>

struct	entry_ref;

class FileListItem : public BStringItem
{
	public:
		FileListItem( entry_ref *ref );
		virtual ~FileListItem( void );
	
	public:
		entry_ref		ref;
};

class FListView : public BListView
{
	public:
		FListView( 	BRect 		frame, 
					const char 	*name, 
					uint32 		resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
					uint32 		flags = B_WILL_DRAW | B_FRAME_EVENTS );
		virtual ~FListView( void );
		
		virtual void Draw( BRect bounds );
		virtual	void MessageReceived( BMessage *msg );
		virtual void SortItems( void );
		virtual void FrameResized( float new_width, float new_height );
}; 

#endif
