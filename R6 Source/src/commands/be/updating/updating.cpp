#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Application.h>
#include <Message.h>
#include <Window.h>
#include <Screen.h>
#include <String.h>
#include <OS.h>

#include "InfoTerminal.h"

class UpdateWindow : public BWindow
{
	public:
		UpdateWindow( const char *info );
		
		virtual	bool QuitRequested();
		void SetInfo( const char *info );
	
	protected:
		InfoTerminal	*infoView;
};

class UpdateApp : public BApplication
{
	public:
		UpdateApp( void );
		
		virtual	void ArgvReceived( int32 argc, char **argv );
		virtual void ReadyToRun( void );
	
	protected:
		BString			info;
		UpdateWindow	*window;
		
};

int main( void )
{
	new UpdateApp();
	be_app->Run();
	delete be_app;
}

UpdateApp::UpdateApp( void )
	: BApplication( "application/x-vnd.Be-UPDTING" )
{
	window = NULL;
}

void UpdateApp::ArgvReceived( int32 argc, char **argv )
{
	if( argc > 1 )
	{
		if( strcmp( argv[1], "--quit" ) == 0 )
			PostMessage( B_QUIT_REQUESTED );
		else
		{
			info.SetTo( argv[1] );
			if( window )
				window->SetInfo( info.String() );
		}
	}
}

void UpdateApp::ReadyToRun( void )
{
	window = new UpdateWindow( info.String() );
}

#define W_HEIGHT	150
#define W_WIDTH		500
#define TEXT_HEIGTH	150

UpdateWindow::UpdateWindow( const char *info )
	: BWindow( BRect( 0, 0, W_WIDTH, W_HEIGHT ), "Update", B_MODAL_WINDOW, B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE )
{
	BRect			sbounds = BScreen().Frame();
	BRect			trect( 0, 0, W_WIDTH, TEXT_HEIGTH );
	BView			*bg;
	
	MoveTo( (sbounds.Width() - W_WIDTH)/2.0, (sbounds.Height() - W_HEIGHT)/2.0 );
	
	bg = new BView( Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS );
	bg->SetViewColor( 216, 216, 216 );
	AddChild( bg );
	
	BFont infoFont;
	infoFont.SetFamilyAndStyle( "ProFontISOLatin1", NULL );
	infoFont.SetSpacing( B_BITMAP_SPACING );
	infoFont.SetSize( 16 );
	
	infoView = new InfoTerminal( trect, "info", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT );
	infoView->SetFont( infoFont );
	infoView->SetOptions( IT_BLUR | IT_FROM_CENTER | IT_SCALE_COLOR | IT_LINEAR );
	infoView->SetApproachRate( 0.05 );
	infoView->SetAlignment( B_ALIGN_LEFT );
	rgb_color bgColor={ 0, 0, 0 }, textColor={ 220, 50, 50 };
	infoView->SetColors( bgColor, textColor, 1.0 );
	AddChild( infoView );
	if( info )
		infoView->SetText( info );
	
	Show();
}

bool UpdateWindow::QuitRequested()
{
	return true;
}

void UpdateWindow::SetInfo( const char *info )
{
	infoView->SetText( info );
}

