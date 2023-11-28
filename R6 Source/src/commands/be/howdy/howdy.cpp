// howdy
//
// A simple shelltool for sending BMessages, optionally with
// simple named parameters.
//
// (A tool in the style of 'hey', but with fewer useless prepositions
// and conjunctions on the command line.)

#include <OS.h>
#include <Application.h>
#include <Roster.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Debug.h>

int main(int argc, char *argv[]) {
	int resultcode = 0;

	BMessage m;
	uint32 msgid = '    '; 		// binary version of m.what
	char msgtext[5]; 			// C string version of m.what
	char rcpt[B_MIME_TYPE_LENGTH]; 	// signature of the recipient of the message
	char *name; 				// holding bin for name-value pairs
	char *type =  NULL; 		// points to optional type specifier
	char *val = NULL; 			// points to the value part of name[]
	BMessenger messenger;
	status_t e;

	// printf("arguments: %d\n", argc);
	if ( argc < 3 ) {
		printf( "usage: howdy <signature> <what> [name[:type]=val]*\n"
				"    type := (string|int32|double), default string\n"
				"    \"Where I come from, we don't say 'hey' ... we say 'howdy'.\"\n"
//				"\n"
//				"    howdy sends BMessages, optionally with (n) named parameters, to\n"
//				"    running apps (specified by signature).\n"
			);
		exit(1);
	}

	union { char string[4]; uint32 value; } swapWhat;

	strncpy(swapWhat.string, argv[2], 4);
	swapWhat.value = B_BENDIAN_TO_HOST_INT32(swapWhat.value);
	msgid = swapWhat.value;

	msgtext[4] = '\0';
	PRINT(("howdy: debug: message (local endianness): '%.4s' (0x%lx)\n", swapWhat.string, msgid));

	m = BMessage(msgid);
	
	int arglen;
	bool done, good, gotType;
	
	for(int i=3;i<argc;i++) {
		arglen = strlen(argv[i]);
		if (arglen <= 0) continue;
		
		name = new char[arglen+1];
		strcpy(name, argv[i]); // grab that argument.  
		
		type = NULL;
		val = name;
		done = false;
		good = false;
		gotType = false;
		while (!done) {
			for(; ( *val != '=' && *val != ':' ); val++) // find the '=' in the string
				if ( *val == '\0' ) // oops, did we run out of string?
					break;
			if ( (*val) == ':' ) {
				(*val) = '\0';
				val++;
				type = val;
				gotType = true;
			} else if ( (*val) == '=' ) {
				(*val) = '\0'; 	// terminate at the end of the property name
				val++; 			// now name points to just the property name,
								// and val points to just the property value
				done = true;
				good = true;
			} else {
				done = true;
			}
		}
		
		if (good) {
			if (type == NULL || (*type) == '\0' || strcmp(type, "string") == 0) {
				if ((e = m.AddString(name,val)) == B_OK) {
					PRINT(("  -> String property: '%s' => '%s'\n", name, val));
				} else {
					fprintf(stderr, "howdy: warning: could not add property '%s': %s\n", name, strerror(e));
				}
			} else if (strcmp(type, "int32") == 0) {
				int32 newval = atoi(val);
				if ((e = m.AddInt32(name, newval)) == B_OK) {
					PRINT(("  -> Integer property: '%s' => '%d'\n", name, newval));
				} else {
					fprintf(stderr, "howdy: warning: could not add property '%s': %s\n", name, strerror(e));
				}
			} else if (strcmp(type, "double") == 0) {
				double newval = atof(val);
				if ((e = m.AddDouble(name, newval)) == B_OK) {
					PRINT(("  -> Double property: '%s' => '%f'\n", name, newval));
				} else {
					fprintf(stderr, "howdy: warning: could not add property '%s': %s\n", name, strerror(e));
				}
			} else {
				fprintf(stderr, "howdy: warning: invalid type specification \"%s\", assuming string\n",
					type);
				if ((e = m.AddString(name,val)) == B_OK) {
					PRINT(("  -> String property: '%s' => '%s'\n", name, val));
				} else {
					fprintf(stderr, "howdy: warning: could not add property '%s': %s\n", name, strerror(e));
				}
			}
		} else {
			fprintf(stderr, "howdy: warning: invalid property specification \"%s\", skipping\n", 
				name);
			done = true;
		}
		
		delete [] name;
	}
	
	strcpy(rcpt, argv[1]);

	bool found_recipient = false;
	
	messenger = BMessenger(rcpt);
	
	if (!messenger.IsValid()) {
		team_info tm;
		app_info info;
		int32 tmcookie;
		
		// ok, maybe this is a symbolic name -- let's look around

		while (get_next_team_info(&tmcookie, &tm) == B_NO_ERROR) {
			/* find the base name of this team, compare it against the argument */
			char* tname = strrchr(tm.args, '/');
			if (tname)
				tname++;
			else
				tname = tm.args;
			if (strcmp(rcpt, tname) == 0) { found_recipient = true; break; }
		}
	
		if (found_recipient && be_roster->GetRunningAppInfo(tm.team, &info) == B_OK) {
			strcpy(rcpt, info.signature);
		}
	} else {
		found_recipient = false;
	}

	if (found_recipient) {
		printf("howdy: using recipient signature: '%s'\n", rcpt);
		messenger = BMessenger(rcpt);
	}

	if (!messenger.IsValid()) {
		fprintf(stderr,"howdy: error: could not build messenger for '%s'.\n", rcpt);
		resultcode = -1;
	} else {
		PRINT(("howdy: debug: sending message ... \n"));

		m.PrintToStream();
		
		status_t result = messenger.SendMessage(&m);
		// delete m;
		printf("howdy: sent message,");
		if ( result == B_OK ) {
			printf(" result=B_OK\n");
		} else {
			printf(" result=%s (%d)\n", strerror(result), (int)result);
		}
	}
	
	exit(resultcode);
}
