#include <stdio.h>
#include <string.h>
#include <fs_info.h>

#include <Entry.h>
#include <Path.h>
#include <Directory.h>

extern "C" int _kstatfs_(dev_t dev, long *pos, int fd, const char *path,
	struct fs_info *fs);

int
main(int argc, char **argv)
{
	status_t	err;
	long		cookie;
	fs_info		info;
	BPath		mount;
	char		fbuf[16];

	if (argc > 1) {
		fprintf(stderr, "usage: df\n  flags:\n");
		fprintf(stderr, "   Q: has query\n");
		fprintf(stderr, "   A: has attribute\n");
		fprintf(stderr, "   M: has mime\n");
		fprintf(stderr, "   S: is shared\n");
		fprintf(stderr, "   R: is removable\n");
		fprintf(stderr, "   W: is writable\n");
		return 1;
	}

	printf("%-16s %-8s %-8s %-8s %-7s %-28s\n",
		"Mount",
		"Type", 
		"Total",
		"Free",
		"Flags",
		"Device");
	printf("%-16s %-8s %-8s %-8s %-5s %-28s\n",
		"----------------",
		"--------",
		"--------",
		"--------",
		"-------",
		"--------------------------");

	cookie = 0;
	while (!_kstatfs_(-1, &cookie, -1, NULL, &info)) {

		BDirectory		dir;
		BEntry			entry;
		node_ref		node;

		node.device = info.dev;
		node.node = info.root;
		err = dir.SetTo(&node);
		if (!err)
			err = dir.GetEntry(&entry);
		if (!err)
			err = entry.GetPath(&mount);
		if (err) {
			printf("problems with volume on %s\n", info.device_name);
			continue;
		}

		sprintf(fbuf, "%c%c%c%c%c%c%c", 
			(info.flags & B_FS_HAS_QUERY) ? 'Q' : '-',
			(info.flags & B_FS_HAS_ATTR) ? 'A' : '-',
			(info.flags & B_FS_HAS_MIME) ? 'M' : '-',
			(info.flags & B_FS_IS_SHARED) ? 'S' : '-',
			(info.flags & B_FS_IS_PERSISTENT) ? 'P' : '-',
			(info.flags & B_FS_IS_REMOVABLE) ? 'R' : '-',
			(info.flags & B_FS_IS_READONLY) ? '-' : 'W');

		printf("%-16s %-8s %8Ld %8Ld %s %-28s\n",
			mount.Path(),
			info.fsh_name,
			(info.total_blocks * info.block_size)/1024,
			(info.free_blocks * info.block_size)/1024,
			fbuf,
			info.device_name);
	}
	return 0;
}
