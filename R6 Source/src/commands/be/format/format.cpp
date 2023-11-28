//--------------------------------------------------------------------
//	
//	format - volume formatting application
//
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include <scsi.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _VOLUME_H
#include <Volume.h>
#endif

class TMountApp : public BApplication {

public:
					TMountApp();
virtual		void	ArgvReceived(int argc, char** argv);
virtual		void	ReadyToRun();

			void	Do();
};

struct media {
	struct media *next;
	char		*user_name;
	char		*dev_name;
};

struct media *mounted_volumes = NULL;
struct media *formattable_devices = NULL;

/* ----------
	GetNumber returns a number obtained from user.
----- */

static int
GetNumber (char *format, ...)
{
	va_list args;
	char b[256];

	va_start(args, format);

	vprintf (format, args);
	va_end(args);
	fflush(stdout);
	gets(b);
	return strtol (b, 0, 0);
}


/* ----------
	add_medium adds a new medium to a list.
----- */
int
add_medium (struct media **list, char *user_name, char *device_name)
{
	struct media	*node;
	int		len;
	int		size;

	len = strlen (user_name) + 1;
	size = sizeof (struct media) + len + strlen (device_name) + 1;

	if (!(node = (struct media *) malloc (size)))
		return -1;

	strcpy ((char *) (node + 1), user_name);
	node->user_name = (char *) (node + 1);
	strcpy ((char *) (node + 1) + len, device_name);
	node->dev_name = (char *) (node + 1) + len;

	node->next = *list;
	*list = node;
	
	return 0;
}


/* ----------
	get_mounted_volumes makes a list of mounted volumes.
----- */
static void
get_mounted_volumes (void)
{
	int			i;
	BVolume		volume;
	char		name[B_OS_NAME_LENGTH];
	char		device[B_OS_NAME_LENGTH];

	for(i=0; volume = volume_at(i), volume.Error() == B_NO_ERROR; i++) {
		volume.GetName(name);
		volume.GetDevice(device);
		add_medium (&mounted_volumes, name, device);
	}
}

	
/* ----------
	add_formattable_device adds the passed device to the list of
	formattable media, if a volume is not already mounted on that
	device and the driver is openable.
----- */
void
add_formattable_device (char *user_name, char *dev_name)
{
	struct media	*node;
	int				id;

	/* see if a volume already mounted on device */
	for (node = mounted_volumes;
	     node && strcmp (dev_name, node->dev_name);
	     node = node->next)
		;

	if (!node) {
		if ((id = open (dev_name, 0)) >= 0) {
			close (id);
			add_medium (&formattable_devices, user_name, dev_name);
		}
	}

	return;
}

	
/* ----------
	get_user_choice prompts the user for which device to format.
----- */
/* static char * */
void
get_user_choice (void)
{
	int 		i;
	int 		choice;
	struct media	*node;

	fprintf (stderr, "\n  device name (user name)\n  ---------------------\n");
	for (;;) {
		for (i = 1, node = formattable_devices;
		     node;
		     node = node->next, i++)
			fprintf (stderr, "  %s (%s)\n", node->dev_name, node->user_name);
		return;
	
		/*
		choice = GetNumber ("Device to format (0 to exit)? ");
		if (choice == 0)
			return NULL;
		if (choice > 0 && choice < i)
			break;
		printf ("Must be a number from 0 to %d\n\n", i-1);
		*/
	}

	/* find the chosen device */
	/*
	for (i = 1, node = formattable_devices;
	     i != choice;
	     node = node->next, i++)
		;

	return node->dev_name;
	*/
}
	

void TMountApp::Do()
{
	char		user_name [100];
	char		*dev_name;
	int			id;
	char		*floppy_driver_name = "/dev/floppy_disk";
	scsi_info	info;


	get_mounted_volumes();		/* make list of mounted volumes */

	add_formattable_device ("floppy", floppy_driver_name);
	add_formattable_device ("ide hard disk - master", "/dev/ide_disk_master");
	add_formattable_device ("ide hard disk - slave", "/dev/ide_disk_slave");

	id = open ("/dev/scsi", 0);
	if (id < 0) {
		fprintf (stderr, "Could not search for scsi disks\n");
	} else {
		/* add each scsi disk driver */
		info.mask = SCSI_DISK_MASK | SCSI_OPTICAL_MASK;
		info.index = 0;
		while (ioctl(id, SCSI_GET_IND_DEVICE, &info) == B_NO_ERROR) {
			sprintf (user_name, "scsi disk w/id %c", info.name[strlen(info.name)-2]);
			add_formattable_device (user_name, info.name);
			info.index++;
		}
		close (id);
	}

	get_user_choice();
	return;

	/* format devices till the cows come home */
	/*
	while (dev_name = get_user_choice()) {
		if (strcmp (dev_name, floppy_driver_name)) {
			printf ("New volume name?");
			fflush (stdout);
			gets (user_name);
		} else
			strcpy (user_name, "fd");
		fflush (stdout);
		if (format_device (dev_name, user_name, 0) < 0)
			printf ("format of '%s' failed\n", dev_name);
		fflush (stdout);
	}
	*/
}

//====================================================================

bool	have_args = FALSE;

int main()
{	
	BApplication* myApp = new TMountApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TMountApp::TMountApp()
		  :BApplication("application/x-vnd.Be-cmd-FRMT")
{
}

//--------------------------------------------------------------------

void TMountApp::ArgvReceived(int argc, char** argv)
{
	int	err;
	int	raw;

	if (argc < 3)
		fprintf (stderr, "format: Insufficient arguments (%d)\n", argc);
	else {
		if (argc > 3) {
			fprintf (stderr, "raw formatting %s...\n", argv[1]);
			raw = TRUE;
		}
		else
			raw = FALSE;
		err = format_device(argv[1], argv[2], raw);
		if (raw)
			fprintf (stderr, "done\n");
		if (err < 0)
			fprintf (stderr, "could not format device %s with volume name %s\n", argv[1], argv[2]);
		have_args = TRUE;
	}
}

//--------------------------------------------------------------------

void TMountApp::ReadyToRun()
{
	if (!have_args) {
		fprintf (stderr, "Usage: device_name volume_name [-r]\n");
		Do();
	}
	PostMessage(B_QUIT_REQUESTED);
}
