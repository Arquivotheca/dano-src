#ifndef QUERY_MENU
#define QUERY_MENU

#include <PopUpMenu.h>
#include <Locker.h>

class BQuery;
class BVolume;
class BLooper;
class QHandler;

class QueryMenu : public BPopUpMenu
{
	friend class QHandler;
	
	public:
		QueryMenu( 	const char *title, 
					bool popUp=false, 
					bool radioMode=false, 
					bool autoRename=false );
		virtual ~QueryMenu( void );
		
		virtual BPoint ScreenLocation( void );
		virtual status_t SetTargetForItems( BHandler *handler );
		status_t SetPredicate( const char *expr, BVolume *vol = NULL );
	
	protected:
		virtual void EntryCreated( const entry_ref &ref, ino_t node );
		virtual void EntryRemoved( ino_t node );
		virtual void RemoveEntries( void );
		
	private:
		virtual void DoQueryMessage( BMessage *msg );
		static int32 query_thread( void *data );
		int32 QueryThread( void );
	
	protected:
		BHandler		*fTargetHandler;
		static BLooper 	*fQueryLooper;
		
	private:
		BLocker			fQueryLock;
		BQuery			*fQuery;
		QHandler		*fQueryHandler;
		bool			fCancelQuery, fPopUp;
		static int32	fMenuCount;
};

#endif
