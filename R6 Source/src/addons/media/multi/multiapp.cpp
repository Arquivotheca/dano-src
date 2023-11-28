#include <MediaRoster.h>
#include <Application.h>
#include <stdio.h>
#include "multi.h"

#ifndef DNDEBUG
#define PRINTF printf
#else
#define PRINTF
#endif

#define TERROR PRINTF


BMediaRoster *MRoster(void);

BMediaRoster *
MRoster()
{
	BMediaRoster *roster = NULL;
	roster = BMediaRoster::Roster();
	if (roster == NULL){
		TERROR("MediaServer is gone!\n");
		debugger("roster is null");
	}
	return roster;
}

extern int BUFFER_FRAMES;
extern int atoi(const char *);

int
main(
	int argc, 
	char * argv[])
{
	if ((argc > 2) || (argc == 2 && argv[1][0] == '-')) {
		fprintf(stderr, "usage: multiapp [ framecount ]\n");
		return 1;
	}
	if (argc == 2) {
		BUFFER_FRAMES = atoi(argv[1]);
		if (BUFFER_FRAMES < 128 || (BUFFER_FRAMES & (BUFFER_FRAMES-1))) {
			fprintf(stderr, "framecount must be >= 128 and power of 2\n");
			return 1;
		}
	}
	BApplication app("application/x-vnd.Be.multiapp");
	MRoster();
	int fd = open("/dev/audio/multi/layla/1", O_RDWR);
	if (fd < 0){
		fd = open("/dev/audio/multi/layla/1", O_RDWR);
	}
	if (fd < 0) {
		TERROR("MultiApp couldn't open driver\n");
		return 1;
	}
	BMultiNode *multi = new BMultiNode("multi", NULL, 0, fd);
	MRoster()->RegisterNode(multi);
	media_node multinode;
	MRoster()->GetNodeFor(multi->ID(), &multinode);
	MRoster()->SetTimeSourceFor(multinode.node, multinode.node);
	MRoster()->SeekTimeSource(multinode, 0, system_time());
	MRoster()->StartTimeSource(multinode, system_time()+20000);
	
	/* wait */
	printf("press any key to quit\n");
	char c;
	scanf("%c", &c);
	
	/* unregister node */
	MRoster()->StopTimeSource(multinode, system_time(), true);
	MRoster()->UnregisterNode(multi);
	delete(multi);
	return 1;
}

