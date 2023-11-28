#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Application.h>
#include <Roster.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <fs_attr.h>
#include <Entry.h>
#include <time.h>
#include <beia_settings.h>

#include <www/ContentView.h>
#include <www/URL.h>
#include <www/ResourceCache.h>
#include <Window.h>
#include <Binder.h>

#include "MCABinderNode.h"

#define NS_CONNECTION_INFO 'qNCI'
#define NS_CONNECTION_DOWN 	1
#define NS_CONNECTION_UP 	2
#define NS_PPP_UP 			3
#define NS_PPP_DOWN 		4
#define NS_ETHER_UP 		5
#define NS_ETHER_DOWN 		6

#define COMMAND_REQUEST		'ScRq'
#define COMMAND_REPLY		'ScRp'
#define COMMAND_SCHED		'Sced'
#define			SCHED_WHEN  "when"
#define		SCHED_INTERVAL  "interval"

#define SCRIPT_STARTED 'Udss'
#define SCRIPT_DONE 'Udsd'
#define SCRIPT_FAILED 'Udsf'

enum
{
	NETS_DOWN,
	NETS_UP,
};

static const char *kNSSig = "application/x-vnd.Be-NETS";
static const char *kUpdtScript = "/etc/update/updatescript";
static const char *kUberUpdtScript = "/etc/update/updt";
static const char *kDoScript = "/etc/update/doscript";
static const char *kUpdtSig = "application/x-vnd.Be-UPDT";

class UpdateApp : public BApplication
{
	public:
		UpdateApp( void );
		
		virtual	void MessageReceived( BMessage *msg );
		virtual	void ArgvReceived( int32 argc, char **argv );
		virtual	void Pulse( void );
		virtual void ReadyToRun( void );
		
		status_t DoUpdate( bool reload, bool ignore_timeout );
		status_t DoUpdateIfNoPeriodic( bool reload );
		int RequestNS( void );
	
	protected:
		status_t	fUpdateStatus;
		bigtime_t	fLastUpdate;
		time_t		fSchedUpdate;
		time_t		fSchedInterval;
		time_t		fIntervalCount;
};

int main(int argc, char **argv)
{
	new UpdateApp();
	be_app->Run();
	delete be_app;
}

UpdateApp::UpdateApp( void )
	: BApplication( kUpdtSig, NULL, true )
{
	char uset[32];

	fUpdateStatus = B_OK;
	fLastUpdate = 0;
	fSchedUpdate=0xffffffff;
	fSchedInterval=0;
	fIntervalCount=0;
	SetPulseRate( 60000000LL ); // 60 Seconds
	
	new MCABinderNode();
}

void UpdateApp::MessageReceived( BMessage *msg )
{
	msg->PrintToStream();
	switch( msg->what )
	{
		case SCRIPT_STARTED:
			printf("Updatescript running\n");
			break;

		case SCRIPT_DONE:
			printf("Updatescript done\n");
			fUpdateStatus = B_OK;
			break;

		case SCRIPT_FAILED:
			printf("Updatescript failed\n");
			fUpdateStatus = B_ERROR;
			break;
			
		case B_SOME_APP_LAUNCHED:
		{
			const char *s;
			s = msg->FindString( "be:signature" );
			
			if( s && (strcmp( s, kNSSig ) == 0) ) {
				if( RequestNS() == NETS_UP ) {
					DoUpdateIfNoPeriodic( true );
				}
			}
			break;
		}
	
		case COMMAND_REQUEST:
		{
			const char 	*cmd;
			const char 	*s;
			char		*env_string;
			
			if( msg->FindString( "command", &cmd ) == B_OK )
			{
				BMessage reply( COMMAND_REPLY );
				
				if( strcmp( cmd, "update" ) == 0 )
				{
					printf( "update command\n" );
					DoUpdate( true, true );
					reply.AddInt32( "result", fUpdateStatus );
				}
				else if( strcmp( cmd, "retry" ) == 0 && fUpdateStatus != B_OK )
				{
					printf( "retry command\n" );
					DoUpdate( false, true );
					reply.AddInt32( "result", fUpdateStatus );
				}
				else if( strcmp( cmd, "doscript" ) == 0 &&
				(s = msg->FindString( "SCRIPT_URL" )) != NULL &&
				(env_string = (char *)malloc(strlen(s) + 32)) != NULL )
				{
					printf("script command\n");
					sprintf( env_string, "SCRIPT_URL=%s", s );
					putenv( env_string );
					free( env_string );
					reply.AddInt32( "result", system( kDoScript ) );
				}
				else if( strcmp( cmd, "report" ) == 0) {
					if (!fSchedInterval)
						reply.AddInt32( SCHED_WHEN,fSchedUpdate );
					else
						reply.AddInt32( SCHED_INTERVAL,fSchedInterval );
					reply.AddInt32( "result", 0 );
				}
				else
					reply.AddInt32( "result", -1 );
				msg->SendReply( &reply );
			}
			break;
		}

		case COMMAND_SCHED:
		{
			time_t tim;
			time_t interval;
			
			printf("sched command\n");
			if( msg->FindInt32( SCHED_WHEN,(int32 *) &tim ) == B_OK ) {
				printf("SCHED_WHEN is %ld\n", tim);
				fSchedUpdate=tim;
				fLastUpdate=0;
				fSchedInterval=0;
			} else if( msg->FindInt32( SCHED_INTERVAL,(int32 *) &interval) == B_OK) {
				printf("SCHED INTERVAL is %ld\n", interval);
				fSchedUpdate=0xffffffff;
				fSchedInterval=interval;
				fLastUpdate=0;
				fIntervalCount=0;
			}
		}
		
		default:
			BApplication::MessageReceived( msg );
			break;
	}
}

void UpdateApp::ArgvReceived( int32 argc, char **argv )
{
	if( argc > 1 )
	{
		if( strcmp( argv[1], "--updt" ) == 0 )
		{
			DoUpdate( true, true );
			return;
		}	
		if( strcmp( argv[1], "--testsched") == 0)
		{
			struct tm tm;
			time_t now;
			BMessage mess(COMMAND_SCHED);
			now = time(0);
			
			printf("scheduling an update for a minute from now\n");
			memset(&tm, 0, sizeof(tm));
			
			if(localtime_r(&now, &tm) != 0)
			{
				now = tm.tm_min + (60 * tm.tm_hour) + 1;
			}
			
			mess.AddInt32(SCHED_WHEN, now);
			be_app->PostMessage(&mess);
			
			return;
		}
		if( strcmp( argv[1], "--testschedinterval") == 0)
		{
			time_t interval;
			BMessage mess(COMMAND_SCHED);
			
			printf("scheduling an update for every 2 minutes\n");

			interval = 2;
			mess.AddInt32(SCHED_INTERVAL, interval);
			be_app->PostMessage(&mess);
			
			return;
		}
	}
}

// Pulse() is called every minute
void UpdateApp::Pulse( void )
{
	struct tm tm;
	time_t now, offset = 0;
	bool periodic = true;
	char uset[32];
	
	now = time(0);
	// XXX - binder node!
	uset[0] = 0;
	get_setting("updated", "no_periodic", uset, 32);
	
	if(uset[0] != '0')
	{
		printf("periodic updating disabled\n");
		periodic = false;
	}
	
	memset(&tm, 0, sizeof(tm));
	
	if(localtime_r(&now, &tm) != 0)
	{
		offset = tm.tm_min + (60 * tm.tm_hour);
	}
	
	printf("fSchedUpdate = %ld, offset = %ld\n", fSchedUpdate, offset);
	
	// update now IF sched updates are disabled
	if(periodic) {
		printf("Pulse sez we be doing update due to periodic flag\n");
		if( RequestNS() == NETS_UP ) 
			DoUpdate( true, false );
		return;
	}	

	if(fSchedInterval > 0) {
		// update the interval counter, if we're configured to run in that mode
		fIntervalCount++; // XXX assumes pulse goes off after 1 minute
		if(fIntervalCount >= fSchedInterval) {
			printf("Pulse sez we be doing update due to interval updates\n");
			if( RequestNS() == NETS_UP ) {
				if( DoUpdate( true, false ) == B_OK) {
					// only reset the interval after a successful update.
					// otherwise, the update will try to trigger on all subsequent Pulse()s
					fIntervalCount = 0;
					return;
				}
			}
		}
	}
	
	// update now if we fall in a 30-minute window after a scheduled update
	if((fSchedUpdate != 0xffffffff) && (offset != 0) && (offset >= fSchedUpdate)
	   && offset < (fSchedUpdate + 30))
	 {
		printf("Pulse sez we be doing update due to scheduled updates\n");
		if( RequestNS() == NETS_UP ) 
			DoUpdate( true, false );
	 }
}

status_t UpdateApp::DoUpdateIfNoPeriodic( bool reload )
{
	char uset[32];

	uset[0] = 0;
	get_setting("updated", "no_periodic", uset, 32);

	if(uset[0] != '1')
	{			
		return DoUpdate( reload, false );
	}
	return B_OK;										
}

status_t UpdateApp::DoUpdate( bool reload, bool ignore_timeout )
{
	bigtime_t deltaT = system_time() - fLastUpdate;
	bool do_update;
	
	do_update = (fLastUpdate == 0) || !reload || ignore_timeout;
	if(do_update == false) {
		// see if we need to check to see if we had already done an update within
		// a 15-minute or 4 hour time window previously
		char uset[32];
		uset[0] = 0;
		get_setting("updated", "no_update_lockout_window", uset, 32);
		if(uset[0] == '1') {
			do_update = true; // override any lockout windows that normally occur
			                  // because of a previous update
		} else {
			do_update = (fUpdateStatus == B_OK ? deltaT > 14400000000LL : deltaT > 900000000LL);
		}
	}

	if(do_update)
	{
		if( reload == true )
			putenv( "RELOAD=true" );
		else if( BEntry(kUpdtScript).Exists() )
			putenv( "RELOAD=false" );
		else
			return fUpdateStatus;
		fUpdateStatus = system( kUberUpdtScript );
		printf( "Update Status=%ld\n", fUpdateStatus );
		fLastUpdate = system_time();
		return fUpdateStatus;
	}
	return B_OK;
}

int UpdateApp::RequestNS( void )
{
#if 0
	BMessenger msgr( kNSSig );
	int retval = NETS_DOWN;
	
	if( msgr.IsValid() )
	{
		BMessage msg( NS_CONNECTION_INFO );
		BMessage reply;
		
		msg.AddString( "appsig", kUpdtSig );
		if( msgr.SendMessage( &msg, &reply ) == B_OK )
		{
			if( reply.what == NS_CONNECTION_INFO ) {
				int32 status;
				if( reply.FindInt32( "status", &status ) == B_OK )
				{
					switch( status )
					{
						case NS_CONNECTION_UP:
						case NS_PPP_UP:
						case NS_ETHER_UP:
							retval = NETS_UP;
							break;
							
						case NS_CONNECTION_DOWN:
							printf( "Network is down.\n" );
							break;
						
						default:
							printf( "NetServer Say: %ld\n", status );
							break;
					}
				}
				else
					printf( "Status field missing!\n" );
			}
		}		
	}
	return retval;
#else
	BinderNode::property  status=  BinderNode::Root() / "service" / "network" / "status";

	return (strcmp(status.String().String(), "up")== 0)?NETS_UP:NETS_DOWN;
#endif
}

void UpdateApp::ReadyToRun( void )
{
	{
		char uset[33];
		time_t interval;

		memset(uset, 0, sizeof(uset));
		uset[0] = '0';
		get_setting("updated", "update_interval", uset, sizeof(uset)-1);
		interval = atol(uset);
		if(interval > 0) {
			printf("update interval (loaded from config file) = %ld\n", interval);
			fSchedInterval = interval;
			fIntervalCount = 0;
		}
	}
	printf( "updated started\n" );
	be_roster->StartWatching( BMessenger( this ), B_REQUEST_LAUNCHED | B_REQUEST_QUIT );
	if(RequestNS() == NETS_UP)
		DoUpdateIfNoPeriodic( true );
}
