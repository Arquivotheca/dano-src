#include <Application.h>
#include <StringView.h>
#include <ListView.h>
#include <ListItem.h>
#include <stdio.h>
#include <ctype.h>
#include "DictWindow.h"
#include "TextControl.h"
#include "WordData.h"
#include "ScrollView.h"
#include "TextView.h"


static const float kWWidth = 500, kWHeight = 250, kWX = 100, kWY = 100;
static const float kDefWidth = 376;
static const char *kWName = "Dictionary";

DictWindow::DictWindow( void )
	: BWindow( BRect( kWX, kWY, kWX+kWWidth, kWY+kWHeight ),
		kWName, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_H_RESIZABLE  )
{
	
}

DictWindow::~DictWindow( void )
{
	
}

bool DictWindow::QuitRequested( void )
{
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}

void DictWindow::MessageReceived( BMessage *msg )
{
	switch( msg->what )
	{
		case MSG_WORD_INVOKED:
			{
				int32		index = msg->FindInt32( "index" );
				BStringItem	*item;
				
				if( (item = (BStringItem *)wordList->ItemAt( index )) )
					wordField->SetText( item->Text() );
			}
			break;
			
		case MSG_WORD_CHANGED:
			{
				FileEntry	*entry;
				
				if( *(wordField->Text()) )
				{
					// Lookup Def
					if( (entry = gDictonary->GetEntry( wordField->Text() )) )
					{
						defBox->SetText( entry->String()+1 );
						delete entry;
					}
					else
						defBox->Delete( 0, defBox->TextLength() );
					
					// Lookup Syns
					if( (entry = gThesaurus->GetEntry( wordField->Text() )) )
					{
						char		buffer[4096];
						char 		*dst = buffer;
						
						// Insert spaces after commas
						for( const char	*src = entry->String()+1; *src; src++ )
						{
							if( *src == ',' )
							{
								*dst++ = *src;
								*dst++ = ' ';
							}
							else
								*dst++ = *src;
						}
						*dst = 0;
						synBox->SetText( buffer );
						delete entry;
					}
					else
						synBox->Delete( 0, synBox->TextLength() );
					
					// Lookup Words
					BList 		matches;
					BString 	*string;
					
					// Remove previous words
					wordList->RemoveItems( 0, wordList->CountItems() );
					
					if( gWords->FindBestMatches( &matches, wordField->Text() ) )
					{
						for( int32 i=0; (string=(BString *)matches.ItemAt(i)); i++ )
						{
							wordList->AddItem( new BStringItem( string->String() ) );
							delete string;
						}
					}
					
				} // end if( *(wordField->Text()) )
				else
				{
					defBox->Delete( 0, defBox->TextLength() );
					synBox->Delete( 0, synBox->TextLength() );
					wordList->RemoveItems( 0, wordList->CountItems() );
				}
			}
			break;
			
		default:
			BWindow::MessageReceived( msg );
			break;
	}
}

status_t DictWindow::InitChildren( void )
{
	BScrollView	*scrollView;
	BRect		frame, textRect;
	
	SetSizeLimits( 80, 1e6, 200, 1e6 );
	// Backgroud
	BView		*mainView = new BView( Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW );
	mainView->SetViewColor( 255, 225, 50 );
	AddChild( mainView );
	
	// Word Field
	wordField = new BTextControl( BRect( 5, 5, kDefWidth-5, 25 ), "Word", "Word:", "",
		new BMessage( MSG_WORD_CHANGED ), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
	wordField->SetTarget( this );
	wordField->SetDivider( 30 );
	wordField->SetModificationMessage( new BMessage( MSG_WORD_CHANGED ) );
	mainView->AddChild( wordField );
	
	// Label
	frame.Set( 5, 30, kDefWidth-5, 50 );
	mainView->AddChild( new BStringView( frame, "", "Definition:" ) );
	
	// Definition Box
	frame.Set( 5, 55, kDefWidth-19, kWHeight-81 );
	textRect.Set( 0, 0, kDefWidth-25, 0 );
	defBox = new BTextView( frame, "", textRect, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS );
	scrollView = new BScrollView( "", defBox, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER );
	mainView->AddChild( scrollView );
	defBox->MakeEditable( false );
	
	// Label
	frame.Set( 5, kWHeight-80, kDefWidth-5, kWHeight-60 );
	mainView->AddChild( new BStringView( frame, "", "Thesaurus:", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM ) );
	
	// Syn Box
	frame.Set( 5, kWHeight-55, kDefWidth-19, kWHeight-5 );
	textRect.Set( 0, 0, kDefWidth-25, 0 );
	synBox = new BTextView( frame, "", textRect, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS );
	scrollView = new BScrollView( "", synBox, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER );
	mainView->AddChild( scrollView );
	synBox->MakeEditable( false );
	
	// Label
	frame.Set( kDefWidth+10, 30, kWWidth-5, 50 );
	mainView->AddChild( new BStringView( frame, "", "Words:" ) );
	
	// Words List
	frame.Set( kDefWidth+10, 55, kWWidth-19, kWHeight-5 );
	wordList = new BListView( frame, "" );
	scrollView = new BScrollView( "", wordList, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER );
	mainView->AddChild( scrollView );
	wordList->SetMessage( new BMessage( MSG_WORD_INVOKED ) );
	
	wordField->MakeFocus( true );
	return B_OK;
}

void DictWindow::FrameResized( float width, float height )
{
	//BRect		textRect;
	
	//textRect.Set( 0, 0, width-25, 0 );
	//defBox->SetTextRect( textRect );
	//synBox->SetTextRect( textRect );
	BWindow::FrameResized( width, height );
}

