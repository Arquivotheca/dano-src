#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <VolumeRoster.h>
#include <fs_info.h>

int
is_meta(char ch)
{
	if (ch == '['  || ch == ']'  || ch == '*'  || ch == '"' || ch == '\'' ||
		ch == '?'  || ch == '^'  || ch == '\\' || ch == ' ' || ch == '\t' ||
		ch == '\n' || ch == '\r' || ch == '('  || ch == ')' || ch == '{'  ||
		ch == '}'  || ch == '!'  || ch == '$'  || ch == '%' || ch == '~'  ||
		ch == '`'  || ch == '@'  || ch == '#'  || ch == '&' || ch == '>'  ||
		ch == '<'  || ch == ';'  || ch == '|')
		return 1;
	
	return 0;
}


char *
escape_meta_chars(char *str)
{
	int len, num_meta = 0;
	char *tmp, *ptr;

	for(len=0, tmp=str; *tmp; tmp++) {
		if (is_meta(*tmp))
			num_meta++;
		else
			len++;
	}

	ptr = (char *)malloc(len + num_meta*2 + 1);
	if (ptr == NULL)
		return ptr;
	
	for(tmp=ptr; *str; str++) {
		if (is_meta(*str)) {
			*tmp++ = '\\';
			*tmp++ = *str;
		} else {
			*tmp++ = *str;
		}
	}

	*tmp = '\0';     /* null terminate */
	
	return ptr;
}


status_t count_block_runs(const char *fname)
{
	int fd;
	status_t result;
	long long pos, bnum, last_bnum;
	uint32 runs;
	fs_info fsinfo;
	struct stat st;

	if ((result = fs_stat_dev(dev_for_path(fname), &fsinfo)) < 0) {
		fprintf(stderr, "error getting block size for '%s' (%s)\n", fname,
			strerror(result));
		return result;
	}

	if (strcmp(fsinfo.fsh_name, "bfs")) {
		fprintf(stderr, "file must be on a bfs file system (%s is on a %s "
			"file system)\n", fname, fsinfo.fsh_name);
		return EINVAL;
	}

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "error opening '%s' (%s)\n", fname, strerror(fd));
		return fd;
	}

	if ((result = fstat(fd, &st)) < 0) {
		fprintf(stderr, "error stat()ing '%s' (%s)\n", fname, strerror(result));
		goto err;
	}

	runs = 0;
	for (pos=0;pos<st.st_size;pos+=fsinfo.block_size) {
		bnum = pos;

		if ((result = ioctl(fd, 10001, &bnum, sizeof(long long))) < 0) {
			fprintf(stderr, "error getting block information for '%s' (%s)\n",
				fname, strerror(result));
			goto err;
		}

		if (pos == 0)
			runs = 1;
		else if (bnum != last_bnum + 1)
			runs++;

		last_bnum = bnum;
	}
	result = runs;

err:
	close(fd);

	return result;
}

int makebootable(char *type, char *path)
{
	char *newpath = escape_meta_chars(path);
	char *command;
	int result;

	command = (char *)malloc(sizeof("makebootable - ") + strlen(type) + 
				strlen(newpath) + 1);
	sprintf(command, "makebootable -alert -%s %s", type, newpath);

	result = system(command);

	free(command);
	free(newpath);

	return result;
}

int main(int argc, char **argv)
{
	BVolumeRoster roster;
	BDirectory root;
	BEntry entry;
	BFile file;
	BVolume volume;
	fs_info info;
	uchar signature[2];
	status_t error;
	bool R3_present = false;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <path-to-zbeos>\n", *argv);
		return -1;
	}

	while ((error = roster.GetNextVolume(&volume)) == B_OK) {
		error = fs_stat_dev(volume.Device(), &info);
		if (error != B_OK) {
			char name[B_FILE_NAME_LENGTH];
			strcpy(name, "(unknown)");
			volume.GetName(name);
			fprintf(stderr, "Error stat'ing volume '%s'\n", name);
			R3_present = true;
			break;
		}

		/* ignore non-bfs partitions */
		if (strcmp(info.fsh_name, "bfs"))
			continue;

		if (info.volume_name[0] == 0)
			strcpy(info.volume_name, "(unknown)");

		error = volume.GetRootDirectory(&root);
		if (error != B_OK) {
			fprintf(stderr, "Error getting root directory for volume '%s'\n",
					info.volume_name);
			R3_present = true;
			break;
		}

		root.FindEntry("beos/system/kernel_intel", &entry, false);
		error = entry.InitCheck();
		/* if it doesn't have a kernel, ignore the volume */
		if (error == ENOENT) continue;

		if (error < 0) {
			R3_present = true;
			break;
		}

		if (file.SetTo(&entry, B_READ_ONLY) != B_OK) {
			fprintf(stderr, "error opening kernel_intel on volume '%s'\n",
					info.volume_name);
			R3_present = true;
			break;
		}

		error = file.Read(&signature, 2);
		if (error < 0) {
			R3_present = true;
			break;
		}

		if (error < 2) continue;

		if ((signature[0] == 'M') && (signature[1] == 'Z')) {
			R3_present = true;
			break;
		}

		file.Unset();
	}

	char command[128];

	if (R3_present) {
		error = count_block_runs(argv[1]);
		if (error <= 12)
			return makebootable("safe", argv[1]);

		BApplication app("application/x-vnd.Be.installbootsector");
		(new BAlert("",
				"A dual-boot R3/R4 system has been detected.  You'll\n"
				"need a boot disk to boot R4.  If you are not running\n"
				"R3, you can safely ignore this message.\n"
				"Please see the documentation for more information\n"
				"about this issue.", "Ok"))->Go();
		return makebootable("full", argv[1]);
	} else {
		return makebootable("full", argv[1]);
	}
}
