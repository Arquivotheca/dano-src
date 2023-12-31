#include "GlobalData.h"
#include "generic.h"

#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/ioctl.h>

/* defined in ProposeDisplayMode.c */
extern status_t create_mode_list(void);

static status_t init_common(int the_fd);


/* Initialization code shared between primary and cloned accelerants */
static status_t init_common(int the_fd) {
	status_t result;
	ram_get_private_data gpd;

	/* memorize the file descriptor */
	fd = the_fd;
	/* set the magic number so the driver knows we're for real */
	gpd.magic = RAM_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, RAM_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	if (result != B_OK) goto error0;

	/* clone the shared area for our use */
	shared_info_area = clone_area("RAM shared info", (void **)&si, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, gpd.shared_info_area);
	if (shared_info_area < 0) {
			result = shared_info_area;
	}

error0:
	return result;
}

/* Clean up code shared between primary and cloned accelrants */
static void uninit_common(void) {
	/* release our copy of the shared info from the kernel driver */
	delete_area(shared_info_area);
	/* more cheap paranoia */
	si = 0;
}

/*
Initialize the accelerant.  the_fd is the file handle of the device (in
/dev/graphics) that has been opened by the app_server (or some test harness).
We need to determine if the kernel driver and the accelerant are compatible.
If they are, get the accelerant ready to handle other hook functions and
report success or failure.
*/
status_t INIT_ACCELERANT(int the_fd) {
	status_t result;
	/* note that we're the primary accelerant (accelerantIsClone is global) */
	accelerantIsClone = 0;

	/* do the initialization common to both the primary and the clones */
	result = init_common(the_fd);

	/* bail out if the common initialization failed */
	if (result != B_OK) goto error0;

	/*
	If there is a possiblity that the kernel driver will recognize a card that
	the accelerant can't support, you should check for that here.  Perhaps some
	odd memory configuration or some such.
	*/

	/*
	This is a good place to go and initialize your card.  The details are so
	device specific, we're not even going to pretend to provide you with sample
	code.  If this fails, we'll have to bail out, cleaning up the resources
	we've already allocated.
	*/
	/* call the device specific init code */
	si->mem_size = 4 * 1024;	/* we only support 4KB right now */

	/* bail out if it failed */
	if (result != B_OK) goto error1;

	/*
	Now would be a good time to figure out what video modes your card supports.
	We'll place the list of modes in another shared area so all of the copies
	of the driver can see them.  The primary copy of the accelerant (ie the one
	initialized with this routine) will own the "one true copy" of the list.
	Everybody else get's a read-only clone.
	*/
	result = create_mode_list();
	if (result != B_OK) goto error2;

	si->fbc.frame_buffer = (void *)(((char *)si->framebuffer) + 0);
	si->fbc.frame_buffer_dma = (void *)(((char *)si->framebuffer_pci) + 0);

	/* bail out if something failed */
	if (result != B_OK) goto error3;

	/* a winner! */
	result = B_OK;
	goto error0;

error3:
error2:
	/*
	Clean up any resources allocated in your device specific initialization
	code.
	*/

error1:
	/*
	Initialization failed after init_common() succeeded, so we need to clean
	up before quiting.
	*/
	uninit_common();

error0:
	return result;
}

/*
Return the number of bytes required to hold the information required
to clone the device.
*/
ssize_t ACCELERANT_CLONE_INFO_SIZE(void) {
	/*
	Since we're passing the name of the device as the only required
	info, return the size of the name buffer
	*/
	return MAX_RAM_DEVICE_NAME_LENGTH;
}


/*
Return the info required to clone the device.  void *data points to
a buffer at least ACCELERANT_CLONE_INFO_SIZE() bytes in length.
*/
void GET_ACCELERANT_CLONE_INFO(void *data) {
	ram_device_name dn;
	status_t result;

	/* call the kernel driver to get the device name */	
	dn.magic = RAM_PRIVATE_DATA_MAGIC;
	/* store the returned info directly into the passed buffer */
	dn.name = (char *)data;
	result = ioctl(fd, RAM_DEVICE_NAME, &dn, sizeof(dn));
}

/*
Initialize a copy of the accelerant as a clone.  void *data points to
a copy of the data returned by GET_ACCELERANT_CLONE_INFO().
*/
status_t CLONE_ACCELERANT(void *data) {
	status_t result;
	char path[MAXPATHLEN];

	/* the data is the device name */
	strcpy(path, "/dev");
	strcat(path, (const char *)data);
	/* open the device, the permissions aren't important */
	fd = open(path, B_READ_WRITE);
	if (fd < 0) {
		result = fd;
		goto error0;
	}

	/* note that we're a clone accelerant */
	accelerantIsClone = 1;

	/* call the shared initialization code */
	result = init_common(fd);

	/* bail out if the common initialization failed */
	if (result != B_OK) goto error1;

	/* get shared area for display modes */
	result = my_mode_list_area = clone_area(
		"RAM cloned display_modes",
		(void **)&my_mode_list,
		B_ANY_ADDRESS,
		B_READ_AREA,
		si->mode_area
	);
	if (result < B_OK) goto error2;

	/* all done */
	result = B_OK;
	goto error0;

error2:
	/* free up the areas we cloned */
	uninit_common();
error1:
	/* close the device we opened */
	close(fd);
error0:
	return result;
}

void UNINIT_ACCELERANT(void) {
	/* free our mode list area */
	delete_area(my_mode_list_area);
	/* paranoia */
	my_mode_list = 0;
	/* release our cloned data */
	uninit_common();
	/* close the file handle ONLY if we're the clone */
	if (accelerantIsClone) close(fd);
}
