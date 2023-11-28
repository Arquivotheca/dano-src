#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <Alert.h>
#include <Button.h>
#include <Debug.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <PrivateScreen.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <StringView.h>
#include <TextView.h>

#include <interface_misc.h>
#include <screen_private.h>
#include <Application.h>
#include <Box.h>

#include "App.h"
#include "Wind.h"
#include "MiniLoader.h"

#define MAX_DEVICES 16
typedef struct ModeInfoRec ModeInfo;
struct ModeInfoRec
{
	ModeInfo *next;
	uint32 xres;
	uint32 yres;
};

typedef struct GfxDeviceRec
{
	uint32 deviceID;
	ModeInfo *first;
} GfxDevice;


MainWindow *theWnd;
ConfigMode *currentMode;
int32 currentDevice;

ResList::ResList( BRect r )
	: BListView( r, "ResolutionList" )
{
}

ResList::~ResList()
{
}

void ResList::SelectionChanged()
{
	int32 index = CurrentSelection();
	if( index < 0 )
		return;
		
	ResList::ListItem *item = (ResList::ListItem *)ItemAt(index);
	
	
	BMenuItem *mi;
	while( (mi = theWnd->hzMenu->RemoveItem((int32)0) ) )
		;//delete mi;
	
	currentMode = (ConfigMode *)item->data;
	ConfigMode *md = currentMode;
	int32 w = md->w;
	int32 h = md->h;
	bool isEnabled = false;
	int ct=0;
	while( md && (md->w==w) && (md->h==h))
	{
		BMessage *msg = new BMessage('hz');
		char buf[64];
		sprintf( buf, "%i", md->hz );
		msg->AddInt32( "hz", ct++ );
		mi = new BMenuItem( buf, msg );
		theWnd->hzMenu->AddItem( mi );
		if( md->enabled )
		{
			mi->SetMarked(true);
			isEnabled = true;
		}
		md = md->next;
	}
	theWnd->hzMenu->SetEnabled( isEnabled );
	theWnd->resEnable->SetValue( isEnabled );

	theWnd->hzField->SetEnabled(true);
	theWnd->monField->SetEnabled(true);
	theWnd->resEnable->SetEnabled(true);
	
}


MainWindow::MainWindow()
	: BWindow(BRect(100, 100, 100 + 400, 100 + 300), "OpenGL Preferences",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE )
{
	theWnd = this;
	BRect r(Bounds());
	r.InsetBy(-1, -1);

	BBox* bg = new BBox(r, "bg", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(bg);

	__glInitDeviceList();

	deviceMenu = new BPopUpMenu("Hardware Devices");
	for( int ct=1; ct < deviceCount; ct++ )
	{
		BMessage *msg = new BMessage('dev');
		msg->AddInt32( "device", ct );
		deviceMenu->AddItem( new BMenuItem( deviceList[ct].name, msg ));
	}
	r.top = 30;
	r.bottom = r.top + 18;
	r.left = 9;
	r.right = r. left + 250;
	deviceField = new BMenuField( r, "", "Devices:", deviceMenu );
	bg->AddChild( deviceField );

	r.top = 55;
	r.bottom = r.top + 100;
	resList = new ResList(r);
	bg->AddChild( resList );

	hzMenu = new BPopUpMenu("Refresh");
	r.top = 180;
	r.bottom = r.top + 18;
	hzField = new BMenuField( r, "", "Refresh Rate:", hzMenu );
	bg->AddChild( hzField );

	r.top = 200;
	r.bottom = r.top + 18;
	resEnable = new BCheckBox( r, "Enable", "Enable Resolution", new BMessage('rese'));
	bg->AddChild( resEnable );
	
	monMenu = new BPopUpMenu("Monitor");
	{
		BMessage msg('mon');
		msg.AddInt32( "mon", 0 );
		monItems[0] = new BMenuItem( "Primary", new BMessage( msg ) );
		monMenu->AddItem( monItems[0] );
		msg.ReplaceInt32( "mon", 1 );
		monItems[1] = new BMenuItem( "Aux 1", new BMessage( msg ) );
		monMenu->AddItem( monItems[1] );
		msg.ReplaceInt32( "mon", 2 );
		monItems[2] = new BMenuItem( "Aux 2", new BMessage( msg ) );
		monMenu->AddItem( monItems[2] );
		msg.ReplaceInt32( "mon", 3 );
		monItems[3] = new BMenuItem( "Aux 3", new BMessage( msg ) );
		monMenu->AddItem( monItems[3] );
	}
	r.top = 220;
	r.bottom = r.top + 18;
	monField = new BMenuField( r, "", "Monitor:", monMenu );
	bg->AddChild( monField );

	if( deviceCount > 1 )
	{
		BMenuItem *item = deviceMenu->ItemAt(0);
		item->SetMarked( true );
		SetToDevice( 1 );
		currentDevice = 1;
	}
	else
	{
		currentDevice = 0;
	}
}


MainWindow::~MainWindow()
{
//	delete deviceMenu;
}

void MainWindow::SetToDevice( int deviceID )
{
	// Empty here
	resList->MakeEmpty();
	
	currentDevice = deviceID;
	
	int prevW = 0;
	int prevH = 0;
	ConfigMode *md = deviceList[currentDevice].firstMode;
	while( md )
	{
		if( (md->w != prevW) && (md->h != prevH) )
		{
			char buf[64];
			sprintf( buf, "%i x %i @ %i bpp", md->w, md->h, md->bpp );
			resList->AddItem( new ResList::ListItem( buf, md ));
			prevW = md->w;
			prevH = md->h;
		}
		md = md->next;
	}
	
	hzField->SetEnabled(false);
	monField->SetEnabled(false);
	resEnable->SetEnabled(false);
	
	monItems[deviceList[deviceID].monitorId]->SetMarked(true);
}


bool MainWindow::QuitRequested()
{
	int ct;
	for( ct=1; ct<deviceCount; ct++ )
	{
		BPath p;
		FILE *f;
		status_t result;
		
		result = find_directory(B_USER_SETTINGS_DIRECTORY, &p);
		if( result < B_OK )
			continue;
		
		result = p.Append( "opengl" );
		if( result < B_OK )
			continue;
		create_directory( p.Path(), 0777 );
	
		result = p.Append( deviceList[ct].filename );
		if( result < B_OK )
			continue;
			
		f = fopen( p.Path(), "w" );
		if( f )
		{
			ConfigMode *cm = deviceList[ct].firstMode;
			while( cm )
			{
				if( cm->enabled )
				{
					fprintf( f, "res %ix%i %i\n", cm->w, cm->h, cm->hz );
				}
				cm = cm->next;
			}
			fprintf( f, "gamma %f,%f,%f\n", deviceList[ct].gamma[0], deviceList[ct].gamma[1], deviceList[ct].gamma[2] );
			fprintf( f, "monitor %i\n", deviceList[ct].monitorId );
			fclose(f);
		}
	}

	theApp->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void MainWindow::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case B_ABOUT_REQUESTED:
			// 	a modal dialog is about to appear
			//	when it goes away, don't let some other
			//	window grab the focus
//			be_app->MessageReceived(m);
			break;
			
		case 'dev':
			{
				int32 id = m->FindInt32( "device" );
				SetToDevice( id );
			}
			break;

		case 'mon':
			{
				int32 id = m->FindInt32( "mon" );
				deviceList[currentDevice].monitorId = id;
			}
			break;

		case 'hz':
			{
				int32 id = m->FindInt32( "hz" );
				int32 w = currentMode->w;
				int32 h = currentMode->h;
				int32 bpp = currentMode->bpp;

				ConfigMode *cm = deviceList[currentDevice].firstMode;
				
				while( cm )
				{
					if( cm && (cm->w==w) && (cm->h==h) && (cm->bpp=bpp) )
					{
						cm->enabled = 0;
						cm = cm->next;
					}
				}

				cm = currentMode;
				while( id-- )
					cm = cm->next;
				cm->enabled = 1;
				resList->SelectionChanged();
			}
			break;
			
		case 'rese':
			{
				if( !currentDevice )
					return;
					
				BCheckBox *i;
				m->FindPointer("source", (void **)&i);
				int32 val = i->Value();
				if( val )
				{
					currentMode->enabled = 1;
				}
				else
				{
					ConfigMode *cm = currentMode;
					int32 w = cm->w;
					int32 h = cm->h;
					while( cm && (cm->w==w) && (cm->h==h) )
					{
						cm->enabled = 0;
						cm = cm->next;
					}
				}
				resList->SelectionChanged();
			}
			
		default:
			BWindow::MessageReceived(m);
	}
}


