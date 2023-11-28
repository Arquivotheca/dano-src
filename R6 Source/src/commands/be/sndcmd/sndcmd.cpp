#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <www/TellBrowser.h>

int main( int32 argc, char **argv )
{
	status_t result = B_ERROR;
	
	if( argc < 3 )
	{
		fprintf( stderr, "Usage: %s target command [param1] [param2] ...\n", argv[0] );
		fprintf( stderr, "Usage: %s target -n numeric_msg_code [param1] [param2] ...\n", argv[0] );
		fprintf( stderr, "Usage: %s target -a alpha_msg_code [param1] [param2] ...\n", argv[0] );
		return result;
	}
	
	BMessenger		msgr( argv[1] );
	BMessage		msg;
	BMessage		reply;

	int firstparam;

	if( !msgr.IsValid() )
	{
		fprintf( stderr, "The target \"%s\" is not valid.\n", argv[1] );
		return result;
	}
	
	if( strcmp( argv[2], "-a" ) == 0 )
	{
		if( argc < 4 || strlen( argv[3] ) != 4 )
		{
			fprintf( stderr, "Requires 4 digit message code: FuBr\n" );
			return result;
		}
		
		char		*what = (char *)&msg.what;
		char		*param = argv[3];
		what[0] = param[3];
		what[1] = param[2];
		what[2] = param[1];
		what[3] = param[0];
		firstparam = 4;		
	}
	else if( strcmp( argv[2], "-n" ) == 0 )
	{
		if( argc < 4 )
		{
			fprintf( stderr, "Requires numeric message code: 123\n" );
			return result;
		}
		
		msg.what = strtol( argv[3], NULL, 0 );
		firstparam = 4;
	}
	else
	{
		msg.what = TB_CMD_REQUEST;
		msg.AddString( "command", argv[2] );
		firstparam = 3;
	}
	for( int32 i=firstparam; i<argc; i++ )
	{
		if ((strcmp(argv[i],"-")==0)&&(i+2<argc)) {
			msg.AddString( argv[i+1], argv[i+2] );
			i+=2;
		} else {
			msg.AddString( "param", argv[i] );
		}
	}
	if( (result = msgr.SendMessage( &msg, &reply )) == B_OK )
	{
		if( reply.what == TB_CMD_REPLY )
			result = reply.FindInt32( "result" );
		else
			fprintf( stderr, "The target \"%s\" did not send a valid reply. (0x%08x)\n", argv[1] , reply.what);
	}
	else
		fprintf( stderr, "Could not send message because: %s\n", strerror(result) );
	
	return result;
}

