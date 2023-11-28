#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <pef.h>

#define		RES_CONTID		'resf'

int
main(int argc, char **argv)
{
	char		rname[256];
	int			rfd, pfd;
	void		*p;
	pef_cheader	pheader;
	pef_sheader	*sheaders;
	int			i;
	int			start, end, eof, size;

	if (argc != 2) {
		fprintf(stderr, "usage: stripres peffile\n");
		exit(1);
	}
	pfd = open(argv[1], O_RDONLY);
	if (pfd < 0) {
		fprintf(stderr, "cannot open pef file %s\n", argv[1]);
		exit(1);
	}

	lseek(pfd, 0, SEEK_SET);
	if (read(pfd, &pheader, sizeof(pheader)) != sizeof(pheader)) {
		fprintf(stderr, "%s is not a pef file\n", argv[1]);
		exit(1);
	}
	if ((pheader.magic1 != PEF_MAGIC1) ||
		(pheader.magic2 != PEF_MAGIC2) ||
		(pheader.cont_id != PEF_CONTID)) {
		fprintf(stderr, "%s is not a pef file\n", argv[1]);
		exit(1);
	}
	start = 0;
	size = pheader.snum * sizeof(pef_sheader);
	sheaders = (pef_sheader *) malloc(size);
	if (read(pfd, sheaders, size) != size) {
		fprintf(stderr, "%s is not a pef file\n", argv[1]);
		exit(1);
	}
	for(i=0; i<pheader.snum; i++) {
		end = sheaders[i].offset + sheaders[i].raw_size;
		if (end > start)
			start = end;
	}
	free(sheaders);
	
	eof = lseek(pfd, 0, SEEK_END);
	if (eof < start) {
		fprintf(stderr, "%s is not a pef file\n", argv[1]);
		exit(1);
	}
	size = eof - start;
	if (size == 0) {
		fprintf(stderr, "%s has no resources\n", argv[1]);
		exit(1);
	}

	sprintf(rname, "%s.rsrc", argv[1]);
	rfd = creat(rname, 0777);
	if (rfd < 0) {
		fprintf(stderr, "cannot create resource file %s\n", rname);
		exit(1);
	}
	p = malloc(size);
	lseek(pfd, start, SEEK_SET);
	if (read(pfd, p, size) != size) {
		fprintf(stderr, "problem reading the pef file\n");
		exit(1);
	}
	lseek(rfd, 0, SEEK_SET);
	pheader.magic1 = PEF_MAGIC1;
	pheader.magic2 = PEF_MAGIC2;
	pheader.cont_id = RES_CONTID;
	if (write(rfd, &pheader, sizeof(pheader)) != sizeof(pheader)) {
		fprintf(stderr, "problem writing the resource file\n");
		exit(1);
	}
	if (write(rfd, p, size) != size) {
		fprintf(stderr, "problem writing the resource file\n");
		exit(1);
	}
	close(rfd);
	close(pfd);
	free(p);
	return 0;
}
