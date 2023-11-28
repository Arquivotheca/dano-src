
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <OS.h>

port_id inPort,outPort;

void InputThread()
{
	char buffer[2048];
	while (1) {
		gets(buffer);
		if (strcmp("bye",buffer) == 0) return;
		status_t r = write_port_etc(inPort,'_cmd',buffer,strlen(buffer)+1,B_TIMEOUT,100000);
	};
};

void OutputThread()
{
	int32 code;
	char buffer[2048];
	while (1) {
		if (read_port(outPort,&code,buffer,2048) < 0) return;
		switch (code) {
			case 'dmsg': // msg from the app_server debugger
				printf("%s",buffer);
				break;
			case 'echo': {
				/*	The app_server is echoing a message back to us, telling us it
					doesn't recognize the command. */
					printf("No such command: \"%s\"\n",buffer);
				break;
			};
			default:
				fprintf(stderr, "- got unknown response 0x%08x\n", code);
				break;
		};
	};
};

char *outportName = "asdbg:outPort";

int main(int argc, char **argv)
{
	thread_id it,ot;
	int32 dummy;
	char portName[80] = "picasso:debugInput";

	if (argc > 1) {
		sprintf(portName,"%s:debugInput",argv[1]);
	};

	printf("asdbg external debugging console (" __DATE__ " @ " __TIME__ ")\n");

	if ((inPort=find_port(portName)) < 0) {
		printf("Waiting for app_server to start up...\n");
		dummy = 10;
		while ((inPort=find_port(portName)) < 0) {
			snooze(100000);
			if (dummy-- == 0) {
				printf("Can't connect to app_server!\n");
				break;
			};
		};
	};
	
	outPort = create_port(100,outportName);
	if (inPort < 0)
		printf("Couldn't find an external debugger-aware app_server!\nGoing into debugging mode only.\n");
	resume_thread(ot=spawn_thread((thread_entry)OutputThread,"OutputThread",B_NORMAL_PRIORITY,NULL));
	write_port(inPort,'oprt',outportName,strlen(outportName)+1);
	resume_thread(it=spawn_thread((thread_entry)InputThread,"InputThread",B_NORMAL_PRIORITY,NULL));
	wait_for_thread(it,&dummy);
	delete_port(outPort);
	
	return 0;
};
