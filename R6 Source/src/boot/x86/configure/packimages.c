#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define RESOLUTION	0
#define PALETTE		1
#define ON			2
#define WIPE		3
#define BACKGROUND	4

#define TYPES		5

struct packentry {
	int size, offset;
};

struct packimagehdr {
	int w, h, x, y;
};

struct packresolution {
	int x, y, bits;
};

struct packentryinfo {
	int size;
	char filename[128];
	int filesize;
	union {
		struct {
			int w, h, x, y;
		} image;
		struct packresolution resolution;
	} x;
};

static struct packentryinfo entries[TYPES+25];
static int backgrounds = 0;

int output_directory()
{
	int offset = (TYPES + backgrounds) * sizeof(struct packentry), i;
	for (i=0;i<TYPES+backgrounds;i++) {
		struct packentry p;
		p.size = entries[i].size;
		p.offset = (entries[i].size) ? offset : entries[i].size;
		fwrite(&p, sizeof(p), 1, stdout);
		offset += entries[i].size;
	}
	return 0;
}

int output_file(const char *filename, int size)
{
	int fd;
	static char buffer[64 * 1024];

	if (!(*filename))
		return 0;

	fd = open(filename, 0);
	if (fd < 0) {
		fprintf(stderr, "error opening image file %s\n", filename);
		return 1;
	}

	while (size) {
		int n = (size > sizeof(buffer)) ? sizeof(buffer) : size;
		if (read(fd, buffer, n) != n) {
			fprintf(stderr, "error reading image file %s\n", filename);
			close(fd);
			return 1;
		}
		fwrite(buffer, n, 1, stdout);
		size -= n;
	}
	
	close(fd);

	return 0;
}

int output_image(struct packentryinfo *ei)
{
	struct packimagehdr h;
	h.w = ei->x.image.w;
	h.h = ei->x.image.h;
	h.x = ei->x.image.x;
	h.y = ei->x.image.y;
	fwrite(&h, sizeof(h), 1, stdout);
	return output_file(ei->filename, ei->filesize);
}

int output_entries(void)
{
	int i;
	
	for (i=0;i<TYPES+backgrounds;i++) {
		struct packentryinfo *ei = entries + i;
		if (i == RESOLUTION) {
			if (ei->size)
				fwrite(&(ei->x.resolution), sizeof(ei->x.resolution), 1, stdout);
		} else if (i == PALETTE) {
			if (ei->filename[0] && output_file(ei->filename, ei->filesize))
				return 1;
		} else {
			if (ei->filename[0] && output_image(ei) != 0)
				return 1;
		}
	}
	return 0;
}

int main()
{
	struct packentry zero;
	char line[128];

	memset(entries + 0, 0, sizeof(entries));
	memset(&zero, 0, sizeof(zero));

	while (fgets(line, sizeof(line), stdin)) {
		struct packentryinfo *ei, entry;
		char type[128], dummy[128];

		line[sizeof(line)-1] = 0;
		if (sscanf(line, "%s", type) != 1)
			continue;

		if (type[0] == '#')
			continue;

		memset(&entry, 0, sizeof(entry));

		if (!strcasecmp(type, "resolution")) {
			if (sscanf(line, "%*s %d %d %d %s",
					&entry.x.resolution.x, &entry.x.resolution.y,
					&entry.x.resolution.bits, dummy) != 3) {
				fprintf(stderr, "bad syntax: resolution x y bits\n");
				return 1;
			}
			ei = entries + RESOLUTION;
			entry.size = 3 * 4;
		} else {
			struct stat st;

			if (!strcasecmp(type, "palette")) {
				ei = entries + PALETTE;
				if (sscanf(line, "%*s %s %s",
						&entry.filename[0], dummy) != 1) {
					fprintf(stderr, "bad syntax: palette filename\n");
					return 1;
				}
			} else {
				if (!strcasecmp(type, "on"))
					ei = entries + ON;
				else if (!strcasecmp(type, "wipe"))
					ei = entries + WIPE;
				else if (!strcasecmp(type, "background")) {
					ei = entries + BACKGROUND + backgrounds;
					backgrounds++;
				} else {
					fprintf(stderr, "invalid type: %s\n", type);
					return 1;
				}
				if (sscanf(line, "%*s %d %d %d %d %s %s",
						&entry.x.image.w, &entry.x.image.h,
						&entry.x.image.x, &entry.x.image.y,
						&entry.filename[0], dummy) != 5) {
					fprintf(stderr, "bad syntax: image width height x y filename\n");
					return 1;
				}

				if (	(entry.x.image.w <= 0) || (entry.x.image.h <= 0) ||
						(entry.x.image.x <= 0) || (entry.x.image.y <= 0)) {
					fprintf(stderr, "nonpositive demension or coordinate for %s\n", entry.filename);
					return 1;
				}
			}
	
			if (stat(entry.filename, &st) < 0) {
				fprintf(stderr, "error getting file size for %s\n", entry.filename);
				return 1;
			}
			if (st.st_size > 0x20000) {
				fprintf(stderr, "sorry, file size for %s is excessive\n", entry.filename);
				return 1;
			}

			entry.filesize = entry.size = st.st_size;
			if (ei != entries + PALETTE)
				entry.size += sizeof(struct packimagehdr);
		}

		if (memcmp(ei, &zero, sizeof(zero))) {
			fprintf(stderr, "duplicate type: %s\n", type);
			return 1;
		}

		*ei = entry;	
	}

	if (!entries[PALETTE].filename[0]) {
		fprintf(stderr, "a palette must be specified\n");
		return 1;
	}

	if (output_directory() != 0) {
		fprintf(stderr, "error writing pack directory\n");
		return 2;
	}

	if (output_entries() != 0) {
		fprintf(stderr, "error writing entries\n");
		return 3;
	}

	return 0;
}
