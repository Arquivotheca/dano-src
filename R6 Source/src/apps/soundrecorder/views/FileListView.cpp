#include <stdio.h>
#include <string.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Font.h>

#include "FileListView.h"


static const char *kDropHereStr = "Drop Files Here";
// *******
// FileListItem
// *******

FileListItem::FileListItem( entry_ref *ref )
	: BStringItem( ref->name )
{
	this->ref.set_name( ref->name );
	this->ref.device = ref->device;
	this->ref.directory = ref->directory;
}

FileListItem::~FileListItem( void )
{
	
}

// *******
// FListView
// *******

FListView::FListView( BRect frame, const char *name, uint32 resizeMask, uint32 flags )
	: BListView( frame, name, B_SINGLE_SELECTION_LIST, resizeMask, flags )
{
	
}

FListView::~FListView( void )
{
	void		*item;
	
	for( int32 i=0; (item=(void *)ItemAt(i)); i++ )
		delete item;
}

void FListView::MessageReceived( BMessage *msg )
{
	switch( msg->what )
	{
		case B_REFS_RECEIVED:
			{
				uint32 			type;
				int32 			count;
				entry_ref 		ref;
				FileListItem	*item, *lastItem = NULL;
				bool			addItem;
				
				BNode			node;
				BNodeInfo		info;
				BMimeType		mimeType, refMimeType( "audio" );
				char			mimeStr[256];
				
				msg->GetInfo( "refs", &type, &count );
				
				if ( type == B_REF_TYPE ) 
				{
					for( int32 i=0; i < count; i++ )
					{
						if( msg->FindRef( "refs", i, &ref ) == B_OK )
						{
							addItem = true;
							for( int32 j=0; (item=(FileListItem *)ItemAt(j)); j++ )
							{
								if( item->ref == ref )
								{
									addItem = false;
									break;
								}
							}
							// If the item has a supertype of "audio"
							if( addItem && (node.SetTo( &ref ) == B_OK)&&(info.SetTo( &node ) == B_OK)
							&&(info.GetType( mimeStr ) == B_OK)&&(mimeType.SetTo( mimeStr ) == B_NO_ERROR)
							&&(refMimeType.Contains( &mimeType )) )
							{
								AddItem( lastItem = new FileListItem( &ref ) );
							}
							
						}
					}
					SortItems();
					
					if( lastItem )
					{
						Select( IndexOf( lastItem ) );
					}
				}
			}
			break;
		
		default:
			BListView::MessageReceived( msg );
			break;
	}
}

static int compare_items( const void *arg1, const void *arg2 );

void FListView::SortItems( void )
{
	BListView::SortItems( &compare_items );
}

void FListView::Draw( BRect bounds )
{
	if( CountItems() )
		BListView::Draw( bounds );
	else
	{
		
		PushState();
		
		font_height		fheight;
		
		//SetHighColor( 255, 225, 100 );
		SetHighColor( 235, 235, 235 );
		FillRect( Bounds() );
		
		be_bold_font->GetHeight( &fheight );
		fheight.leading = fheight.ascent + fheight.descent+1;
		
		
		MovePenTo( (Frame().right-Frame().left - be_bold_font->StringWidth( kDropHereStr ))/2.0, 
			(Frame().bottom - Frame().top + fheight.leading)/2.0 );
		SetHighColor( 0, 0, 0 );
		SetFont( be_bold_font );
		DrawString( kDropHereStr );
		
		PopState();
	}
}

static int compare_items( const void *arg1, const void *arg2 )
{
	entry_ref		*ref1, *ref2;
	
	ref1 = &(*(FileListItem **)arg1)->ref;
	ref2 = &(*(FileListItem **)arg2)->ref;
	
	return strcasecmp( ref1->name, ref2->name );
}

void FListView::FrameResized( float new_width, float new_height )
{
	if( CountItems() )
		BListView::FrameResized( new_width, new_height );
	else
		Draw( Bounds() );
}
