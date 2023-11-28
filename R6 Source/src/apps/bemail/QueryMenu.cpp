#include "QueryMenu.h"
#include <Query.h>
#include <Autolock.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>
#include <Looper.h>
#include <Node.h>
#include <stdio.h>
#include <string.h>
#include <PopUpMenu.h>

BLooper *QueryMenu::fQueryLooper = NULL;
int32 QueryMenu::fMenuCount = 0;

// ***
// QHandler
// ***

class QHandler : public BHandler
{
	public:
		QHandler( QueryMenu *queryMenu );
		virtual void MessageReceived( BMessage *msg );
		QueryMenu *fQueryMenu;
};

QHandler::QHandler( QueryMenu *queryMenu )
	: BHandler( (const char *)NULL ),
	fQueryMenu( queryMenu )
{
	
}

void QHandler::MessageReceived( BMessage *msg )
{
	switch( msg->what )
	{
		case B_QUERY_UPDATE:
			fQueryMenu->DoQueryMessage( msg );
			break;
		
		default:
			BHandler::MessageReceived( msg );
			break;
	}
}

// ***
// QueryMenu
// ***

QueryMenu::QueryMenu( const char *title, bool popUp, bool radioMode, bool autoRename )
	: BPopUpMenu( title, radioMode, autoRename ),
	fTargetHandler( NULL ),
	fPopUp( popUp )
{
	if( atomic_add( &fMenuCount, 1 ) == 0 )
	{
		fQueryLooper = new BLooper( "Query Watcher" );
		fQueryLooper->Run();
	}
	fQueryHandler = new QHandler( this );
	fQueryLooper->Lock();
	fQueryLooper->AddHandler( fQueryHandler );
	fQueryLooper->Unlock();
	BMessenger mercury( fQueryHandler, fQueryLooper );
	fQuery = new BQuery();
	fQuery->SetTarget( mercury );
}

QueryMenu::~QueryMenu( void )
{
	fCancelQuery = true;
	fQueryLock.Lock();
	delete fQuery;
	fQueryLock.Unlock();
	
	fQueryLooper->Lock();
	fQueryLooper->RemoveHandler( fQueryHandler );
	delete fQueryHandler;
	
	if( atomic_add( &fMenuCount, -1 ) == 1 )
	{
		fQueryLooper->Lock();
		fQueryLooper->Quit();
	}
	else
		fQueryLooper->Unlock();
}

void QueryMenu::DoQueryMessage( BMessage *msg )
{
	int32		opcode;
	int64		directory;
	int32		device;
	int64		node;
	const char 	*name;
	
	if( (msg->FindInt32( "opcode", &opcode ) == B_OK)&&
		(msg->FindInt64( "directory", &directory ) == B_OK)&&
		(msg->FindInt32( "device", &device ) == B_OK)&&
		(msg->FindInt64( "node", &node ) == B_OK) )
	{
		BMessage *tmsg;
		
		if( (opcode == B_ENTRY_CREATED)&&(msg->FindString( "name", &name ) == B_OK) )
		{
			entry_ref ref( device, directory, name );
			EntryCreated( ref, node );
			return;
		}
		else if( opcode == B_ENTRY_REMOVED )
		{
			BAutolock lock( fQueryLock );
			if( !lock.IsLocked() )
				return;
			EntryRemoved( node );
		}
	}
}

status_t QueryMenu::SetPredicate( const char *expr, BVolume *vol )
{
	status_t		status;
	
	// Set the volume
	if( !vol )
	{
		BVolumeRoster rosta;
		BVolume v;
		rosta.GetBootVolume( &v );
		
		if( (status = fQuery->SetVolume( &v )) != B_OK )
			return status;
	}
	else if( (status = fQuery->SetVolume( vol )) != B_OK )
		return status;
	
	if( (status = fQuery->SetPredicate( expr )) == B_OK )
	{
		// Force query thread to exit if still running
		fCancelQuery = true;
		fQueryLock.Lock();
		// Remove all existing menu items (if any... )
		RemoveEntries();
		fQueryLock.Unlock();
		
		// Resolve Query/Build Menu in seperate thread
		thread_id tid;
		tid = spawn_thread( query_thread, "query menu thread", B_NORMAL_PRIORITY, this  );
		return resume_thread( tid );
	}
	return status;
}

void QueryMenu::RemoveEntries( void )
{
	int64 node;
	for( int32 i=CountItems()-1; i>=0; i-- )
	{
		if(ItemAt(i)->Message()->FindInt64( "node", &node ) == B_OK)
			RemoveItem(i);
	}
}

int32 QueryMenu::query_thread( void *data )
{
	return ((QueryMenu *)(data))->QueryThread();
}

int32 QueryMenu::QueryThread( void )
{
	BAutolock	lock( fQueryLock );
	
	if( !lock.IsLocked() )
		return B_ERROR;
	fCancelQuery = false;
	
	// Begin resolving query
	fQuery->Fetch();
	
	entry_ref 		ref;
	node_ref		node;
	
	// Build Menu
	// The leash it a work-around for when the something in the query breaks and prevents
	// GetNextRef() from ever returning B_ENTRY_NOT_FOUND
	for( int32 leash=0; (fQuery->GetNextRef( &ref ) != B_ENTRY_NOT_FOUND)&&(!fCancelQuery)&&(leash < 128); leash++ )
	{
		BEntry entry( &ref );
		entry.GetNodeRef( &node );
		EntryCreated( ref, node.node );
	}
	
	return B_OK;
}

status_t QueryMenu::SetTargetForItems( BHandler *handler )
{
	fTargetHandler = handler;
	return BMenu::SetTargetForItems( handler );
}

void QueryMenu::EntryCreated( const entry_ref &ref, ino_t node )
{
	BMessage 		*msg;
	BMenuItem 		*item;
	
	msg = new BMessage( B_REFS_RECEIVED );
	msg->AddRef( "refs", &ref );
	msg->AddInt64( "node", node );
	item = new BMenuItem( ref.name, msg );
	if( fTargetHandler )
		item->SetTarget( fTargetHandler );
	AddItem( item );
}

void QueryMenu::EntryRemoved( ino_t node )
{
	BMenuItem 	*item;
	int64		inode;
			
	// Search for item in menu
	for( int32 i=0; (item=ItemAt(i)); i++)
	{
		// Is it our item?
		if( ((item->Message())->FindInt64( "node", &inode ) == B_OK)&&
			(inode == node) )
		{
			RemoveItem(i);
			return;
		}
	}
}

BPoint QueryMenu::ScreenLocation(void)
{
	if( fPopUp )
		return BPopUpMenu::ScreenLocation();
	else
		return BMenu::ScreenLocation();
}
