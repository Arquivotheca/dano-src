#define DEBUG 1

#include <Debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <SupportDefs.h>
#include <Message.h>

#include <Application.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <Entry.h>
#include <File.h>
#include <Directory.h>

bool	gDeleteFlag = false;

#if 0
status_t delete_attrs(BEntry *entry)
{
	char	attr_name[256];

	while (node->GetNextAttrName(attr_name) == B_OK) {
		node->RemoveAttr(attr_name);
	}
	return B_OK;
}
#endif

status_t update(BEntry *entry)
{
	status_t	err;
	char		path[1024];

	err = entry->GetPath(path, sizeof(path));
//+	PRINT(("file %s\n", path));
	if (err < 0)
		return err;

//+	if (gDeleteFlag)
//+		return delete_attrs(entry);
//+	else
		return update_mime_info(path, -1, NULL, 0);
}

status_t update(BDirectory *parent, bool recurse);

status_t update(BDirectory *parent, BEntry *entry, bool recurse)
{
	char		name[256];
	BDirectory	dir;
	status_t	err;

	entry->GetName(name);
	if (!parent->IsDirectory(name)) {
		update(entry);
	} else {
		err = dir.SetTo(entry);
		if (!err) {
			char path[1024];
			err = entry->GetPath(path, sizeof(path));
			printf("directory: %s\n", path);
			update(&dir, recurse);
		}
	}
	return B_OK;
}

status_t update(BDirectory *parent, bool recurse)
{
	BEntry		entry;

	while (parent->GetNextEntry(&entry, FALSE) == B_OK) {
		update(parent, &entry, recurse);
	}
	return B_OK;
}

status_t update(BVolume *vol)
{
	char		name[256];
	BDirectory	dir;
	status_t	err;

	err = vol->GetName(name);
//+	PRINT(("Vol: err=%x, name=%s, read_only=%d, Removable=%d, attrs=%d\n",
//+		err, name, vol->IsReadOnly(), vol->IsRemovable(), vol->KnowsAttr()));

	if (vol->KnowsAttr()) {
		err = vol->GetRootDirectory(&dir);
//+		PRINT(("Root Directory: err=%x\n", err));
		if (!err)
			update(&dir, true);
	}
	return B_OK;
}

void walk_volumes()
{
	BVolumeRoster	vr;
	BVolume			vol;

	while (1) {
		vol = vr.NextVolume();
		if (!vol.IsValid())
			break;
		update(&vol);
	}
}

main(int argc, char *argv[])
{
	BApplication	app("application/x-vnd.Be-cmd-NSTF");

	status_t	err = 0;

	if ((argc > 1) && (strcmp(argv[1], "-d") == 0)) {
		gDeleteFlag = true;
		argc--;
		argv++;
	}

	if (argc == 1) {
		walk_volumes();
	} else {
		BEntry		entry(argv[1]);
		BDirectory	dir;
		err = entry.GetParent(&dir);

		if (!err)
			err = update(&dir, &entry, TRUE);
	}

	return err;
}

