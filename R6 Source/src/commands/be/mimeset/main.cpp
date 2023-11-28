#define DEBUG 1

#include <Debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Entry.h>
#include <Mime.h>
#include <Path.h>
#include <Roster.h>

char *arg0;

static void
usage (int status)
{
	if (status == -1) {
		fprintf(stderr, "Try `%s --help' for more information.\n", arg0);
	} else {
	
		app_info appInfo; 
		BFile file; 
		BAppFileInfo appFileInfo; 
	   
		be_app->GetAppInfo(&appInfo); 
		file.SetTo(&appInfo.ref, B_READ_WRITE); 
		appFileInfo.SetTo(&file);
		
		status_t ret;
		version_info info;
		version_kind kind;
		ret = appFileInfo.GetVersionInfo(&info, kind);
		
		
		if (status == 0) {
			printf("Usage: %s [OPTION]... [PATH]...\n", arg0);
			
			printf("\n\
  -all			combine default action and the -apps option\n\
  -apps			update 'app' and 'meta_mime' information\n\
  -f			force updating, even if previously updated\n\
				(will not overwrite the 'type' of a file)\n\
  -F			force updating, even if previously updated\n\
				(will overwrite the 'type' of a file)\n\
  --help		display this help information\n\
  --version		display version number\n\
When PATH is @, file names are read from stdin\n\n\
");
		}

		if (ret == B_OK) {
			printf("Version: %s\n", info.long_info);
		} else {
			printf("Version unknown!\n");
		}
	}

	exit(status);
}

int
main(int argc, char *argv[])
{
	status_t		err = B_OK;
	BApplication	app("application/x-Be.vnd.mimeset",&err);
	if (err != B_OK) {
		fprintf(stderr,"%s : error creating BApplication : %s\n"
					,argv[0],strerror(err));
		exit(1);
	}
	err = 0;
	int				force = 0;
	bool			apps_only = false;
	bool			basic_update = true;
	char			*file = NULL;
	int32			i = 1;
	int			x_stat = 0;
	bool			from_stdin = false;

	arg0 = argv[0];

	while (i < argc) {
		if (strcmp(argv[i], "-f") == 0) {
			force = 1;
		} else if (strcmp(argv[i], "-F") == 0) {
			force = 2;
		} else if (strcmp(argv[i], "-all") == 0) {
			apps_only = true;
			basic_update = true;
		} else if (strcmp(argv[i], "-apps") == 0) {
			apps_only = true;
			basic_update = false;
		} else if (strcmp(argv[i], "--help") == 0) {
			usage(0);
		} else if (strcmp(argv[i], "--version") == 0) {
			usage(1);
		} else if (*(argv[i]) == '-') {
			fprintf(stderr, "unknown option \"%s\"\n", argv[i]);
			usage(-1);
		} else {
			break; /* process files */
		}
		i++;
	}

	if (!argv[i]) {
//+		PRINT(("ALL, apps_only=%d, force=%d\n", apps_only, force));
		if (basic_update)
			err = update_mime_info(NULL, true, true, force);
		if (apps_only)
			err = create_app_meta_mime(NULL, true, true, force);
		x_stat = (err != B_OK);
	} else while (i < argc) {
		static char path[1024];
		if (!from_stdin) {
			file = argv[i];
			if (!strcmp(file, "@")) {
				from_stdin = true;
			}
		}
		if (from_stdin) {
			file = path;
	again:
			path[0] = 0;
			fgets(path, 1024, stdin);
			if (path[0] == '\n') {
				goto again;
			}
			if (!path[0]) {
				from_stdin = false;
				i++;
				continue;
			}
			path[strlen(path)-1] = 0; /* remove trailing \n */
		}
		BEntry	entry(file);
		err = entry.InitCheck();
		if (err == B_OK) {
			BPath	path;
			err = entry.GetPath(&path);
			if (err == B_OK) {
//+				PRINT(("file=%s, apps_only=%d, force=%d\n", file, apps_only, force));
				if (basic_update)
					err = update_mime_info(path.Path(), true, true, force);
				if (apps_only)
					err = create_app_meta_mime(path.Path(), false, true, force);
			}
		}
		if (err < B_OK) {
			fprintf(stderr, "mimeset error: %s '%s'\n", strerror(err), file);
			x_stat++;
		}
		if (!from_stdin) {
			i++;
		}
	}

	return x_stat;
}
