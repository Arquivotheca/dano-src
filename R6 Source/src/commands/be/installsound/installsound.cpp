
#include <Application.h>
#include <Entry.h>
#include <MediaFiles.h>
#include <Path.h>

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>


int
main(
	int argc,
	char * argv[])
{
	if ((argc != 3) && ((argc != 2) || strcmp(argv[1], "--list"))) {
		fprintf(stderr, "installsound eventname filename\n");
		fprintf(stderr, "\tinstalls a new named sound event in the Sounds preferences panel.\n");
		fprintf(stderr, "installsound --list\n");
		fprintf(stderr, "\tlists all sound events.\n");
		fprintf(stderr, "installsound --test eventname\n");
		fprintf(stderr, "\tprints the file for the given event name, or nothing and returns error if none.\n");
		fprintf(stderr, "installsound --add eventname\n");
		fprintf(stderr, "\tinstalls empty named sound event in the Sounds preferences panel.\n");
		fprintf(stderr, "installsound --clear eventname\n");
		fprintf(stderr, "\tclears a named event in the Sounds preferences panel.\n");
		fprintf(stderr, "installsound --remove eventname\n");
		fprintf(stderr, "\tremoves a named event from the Sounds preferences panel.\n");
		return 1;
	}
	BApplication app("application/x-vnd.be.installsound");
	app.PostMessage(B_QUIT_REQUESTED);

	BMediaFiles f;
	BPath p;
	status_t err = B_OK;
	if ((err = f.RewindRefs(BMediaFiles::B_SOUNDS)) < B_OK) {
		fprintf(stderr, "MediaFiles error: %s\n", strerror(err));
	}
	else if (!strcmp(argv[1], "--list")) {
		BString name;
		entry_ref ref;
		while (f.GetNextRef(&name, &ref) == B_OK) {
			BEntry ent(&ref);
			p.Unset();
			ent.GetPath(&p);
			fprintf(stdout, "%s:\t%s\n", name.String(), p.Path());
		}
	}
	else if (!strcmp(argv[1], "--test")) {
		entry_ref ref;
		err = f.GetRefFor(BMediaFiles::B_SOUNDS, argv[2], &ref);
		if (err == B_OK) {
			BEntry ent(&ref);
			err = ent.GetPath(&p);
		}
		if (err == B_OK) {
			struct stat st;
			if (stat(p.Path(), &st) < 0) {
				err = errno;
			}
		}
		if (err == B_OK) {
			fprintf(stdout, "%s\n", p.Path());
		}
		else {
			fprintf(stderr, "%s: %s\n", argv[2], strerror(err));
		}
	}
	else if (!strcmp(argv[1], "--clear")) {
		entry_ref ref;
		err = f.GetRefFor(BMediaFiles::B_SOUNDS, argv[2], &ref);
		if (err == B_OK) {
			err = f.RemoveRefFor(BMediaFiles::B_SOUNDS, argv[2], ref);
		}
		if (err < B_OK) {
			fprintf(stderr, "remove %s: %s\n", argv[2], strerror(err));
		}
	}
	else if (!strcmp(argv[1], "--remove")) {
		err = f.RemoveItem(BMediaFiles::B_SOUNDS, argv[2]);
		if (err < B_OK) {
			fprintf(stderr, "remove %s: %s\n", argv[2], strerror(err));
		}
	}
	else if (!strcmp(argv[1], "--add")) {
		entry_ref ref;
		if (f.GetRefFor(BMediaFiles::B_SOUNDS, argv[2], &ref) != B_OK) {
			err = f.SetRefFor(BMediaFiles::B_SOUNDS, argv[2], ref);
			if (err < B_OK) {
				fprintf(stderr, "add %s: %s\n", argv[2], strerror(err));
			}
		} else {
			fprintf(stderr, "add %s: sound event already exists\n", argv[2]);
		}
	}
	else {	//	install
		entry_ref ref;
		struct stat st;
		if (((err = get_ref_for_path(argv[2], &ref)) < B_OK) ||
				((stat(argv[2], &st) < 0) && (err = errno))) {
			fprintf(stderr, "error: %s not found\n", argv[2]);
		}
		else {
			err = f.SetRefFor(BMediaFiles::B_SOUNDS, argv[1], ref);
			if (err < B_OK) {
				fprintf(stderr, "install %s: %s\n", argv[1], strerror(err));
			}
		}
	}
	app.Run();
	return (err < B_OK) ? 1 : 0;
}
