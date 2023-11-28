#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ctype.h>
#include <pwd.h>
#include <string.h>
#include <OS.h>

status_t nmb_lookup( char *name, struct in_addr *addr );

typedef struct parms_t {
	char Unc[256];
	char Server[256];
	ulong	server_inetaddr;
	char Share[256];
	char Username[256];
	char Password[256];
	char Myname[256];
	char LocalShareName[256];
	char LocalWorkgroup[64];
} parms_t;



void
usage() {

	printf(
		"Usage for cifsmount\n"
		"cifsmount //server/share mountpoint <password> <username> [options]\n"
		"\n"
		"\t-I Server_IP_Address Specify the ip address of the server\n"
		"\tyou're contacting, handy if you dont have a DNS server.\n"
		"\tIf not specified, the server will be looked up via DNS.\n"
		"\n"
		"\t-W Clients_Workgroup Specify which workgroup you're in.\n"
		"\tIf not specified, obtained from your network settings file\n"
		"\t(usually /boot/home/config/settings/network).\n"
		"\n"
		"\t-d Dump out info before attempting to mount the server\n"
	);
	exit(-1);
}


void showparms(parms_t *parms) {
	struct in_addr addr;
	addr.s_addr = parms->server_inetaddr;

	printf("Parameters for mounting are:\n");
	printf("Unc\t\t\t%s\n", parms->Unc);
	printf("Server\t\t\t%s\n", parms->Server);
	printf("Server IP\t\t%s\n", inet_ntoa(addr));
	printf("Share\t\t\t%s\n", parms->Share);
	printf("Username\t\t%s\n", parms->Username);
	printf("Password\t\t%s\n", parms->Password);
	printf("Hostname\t\t%s\n", parms->Myname);
	printf("LocalShareName\t\t%s\n", parms->LocalShareName);
	printf("My workgroup\t\t%s\n", parms->LocalWorkgroup);
	
}

void
strtoupper(char* string) {
	size_t i;
	for(i = 0; i <= strlen(string); i++) {
		string[i] = toupper(string[i]);
	}
}


int
parse_unc(parms_t *parms) {

	char*		share=NULL;
	char*		server=NULL;
	char*		start=NULL;
	char		unc[256];
	int			result = -1;
	
	if (parms != NULL) {
	
		strncpy(unc, parms->Unc, 256);
		unc[255] = '\0';
		
		start = unc;
		server = strtok(start, "\\");
		if ((server == NULL) || (server == start)) {
			printf( "unspecified server\n" );
			usage();
		}
		strcpy(parms->Server, server);
		
		share = strtok(NULL, "\\");
		if (share == NULL) {
			printf( "unspecified share\n" );
			usage();			
		}
		
		
		strcpy(parms->Share, share);
		result = 0;

	}

	return result;
}


int main(int argc, char** argv) {
	int	 			result = 0;
	int				pcount;
	struct hostent	*host_entry = NULL;
	parms_t			parms;
	char			mountpt[1024];
	bool			debug = false;
	
	if( argc == 1 )
		usage();
	
	memset(& parms, 0, sizeof(parms_t));

	for (argc--, argv++; argc > 0; argc -= pcount, argv += pcount) {
		if( **argv == '-' )
		{
			switch( (*argv)[1] )
			{
				case 'I':
					pcount = 2;
					if (argc == 1) usage();
					parms.server_inetaddr = inet_addr(argv[1]);
					if (parms.server_inetaddr == 0) {
						printf("Malformed IP address\n");
						usage();
					}
					break;
				case 'W':
					pcount = 2;
					if (argc == 1) usage();
					strncpy(parms.LocalWorkgroup, argv[1], 64);
					break;
				case 'd':
					pcount = 1;
					debug = true;
					break;
				default:
					pcount = 1;
					if( strcmp( *argv, "--help" ) != 0 )
						printf( "Unrecognized option '%s'\n", *argv );
					usage();
			}
		}
		else
		{
			char *s;
			
			for( pcount = 0; pcount < argc && *(argv[pcount]) != '-'; pcount++ ) { }
			
			if( pcount < 2 )
			{
				printf( "insufficient arguments\n" );
				usage();
			}
			
			strcpy(parms.Unc, argv[0]);
			for( s = parms.Unc; *s; s++ )
			{
				if( isalpha(*s) )
					*s = toupper( *s );
			}
				
			if( parms.Unc[0] == '/' )
			{
				for( s = parms.Unc; *s; s++ )
				{
					if( *s == '/' )
						*s = '\\';
				}
			}
			
			parse_unc(&parms);
			
			strncpy( mountpt, argv[1], 1024 );
			
			if( pcount > 2 )
				strncpy(parms.Password, argv[2], 255);
			
			if( pcount > 3 )
				strncpy(parms.Username, argv[3], 256);
		}
	}
	
	
	if( parms.Unc[0] != '\0' )
	{
		if( parms.Username[0] == '\0' )
		{
			struct passwd *pw;
			
			if( (pw = getpwuid(getuid())) != NULL && pw->pw_name != NULL )
				strncpy(parms.Username, pw->pw_name, 256);
			else
			{
				perror( "could not determine default user name\n" );
				exit(-1);
			}
		}
		
		gethostname(parms.Myname, 255);
		
		if (parms.server_inetaddr == 0) {
			struct in_addr addr;
			
			if( nmb_lookup( parms.Server, &addr ) < 0 )
			{
				host_entry = gethostbyname(parms.Server);
				if (host_entry == NULL) {
					printf("name look up failed on %s\n", parms.Server);
					exit(-1);
				}
				memcpy(& (parms.server_inetaddr), host_entry->h_addr, host_entry->h_length);
			}
			else
				parms.server_inetaddr = addr.s_addr;
				
		}
	
		if (parms.LocalWorkgroup == 0)
			strcpy( parms.LocalWorkgroup, "WORKGROUP" );
		
		if (debug)
			showparms(& parms);	
	
		result = mount("cifs", mountpt, NULL, 0, & parms, 0);
		
		if (result == B_OK) {
			printf("Successfully mounted\n");
		} else {
			printf("Didn't mount, error is %d %s\n", result, strerror(result));
		}
	}
	
	return B_OK;
}
