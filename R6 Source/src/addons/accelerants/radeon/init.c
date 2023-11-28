#include <OS.h>
#include <KernelExport.h>

#include <add-ons/graphics/Accelerant.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include "proto.h"
#include "cursor.h"



int32 primDevFD = -1;
__mem_AreaDef *primMemMgr = 0;
area_id dataArea;
CardInfo *ci = 0;

status_t __radeon_Init (int32 the_fd)
{
	status_t retval = B_OK;
	int32 ret;
	radeon_getglobals gg;

	dprintf (("Radeon_accel: init - ENTER\n"));
	primDevFD = the_fd;

	gg.ProtocolVersion = RADEON_IOCTLPROTOCOL_VERSION;
	if ((retval = ioctl (primDevFD, RADEON_IOCTL_GETGLOBALS, &gg, sizeof (gg))) < 0)
	{
		dprintf (("Radeon_accel: init - Failed to get globals.\n"));
		return (retval);
	}

	// Construct an I2C Bus object to communicate with the card
//	construct_i2c (devfd);

	if ((dataArea = clone_area ("Radeon accel driver data: share", (void **) &ci,
		  B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gg.GlobalArea)) < 0)
	{
		dprintf (("Radeon_accel: init - Failed to clone global area.\n"));
		return B_ERROR;
	}

	dprintf (("Radeon_accel: init - InitClient.\n"));
	primMemMgr = __mem_InitClient (primDevFD);
	dprintf (("Radeon_accel: MemMgr_2D %p\n", primMemMgr ));

//	ret = Radeon_CPInit (ci, CSQ_MODE_PRIPIO_INDDIS);
	ret = Radeon_CPInit (ci, CSQ_MODE_PRIBM__INDBM);
	if ( ret != CCE_SUCCESS )
	{
		dprintf(( "Radeon_accel CCE Init failed !!!  code= 0x%x\n", ret));
	}
	
	cursorCreate();

	ci->benEngineSem = create_sem( 0, "DirectGLWindow_Current_Sem" );
	ci->benEngineInt = 0;

//	dprintf (("Radeon_accel: init - returning 0x%x\n", ci_areaid < 0 ? ci_areaid : B_OK));
	return B_OK;
}

void __radeon_Uninit ()
{
	dprintf (("Radeon_accel: uninit - ENTER\n"));

	if (dataArea >= 0)
	{
		delete_area (dataArea);
		dataArea = -1;
		ci = NULL;
	}
	
	close( primDevFD );
	primDevFD = -1;

/*
	destroy_i2c ();
*/
	dprintf (("Radeon_accel: uninit - EXIT\n"));
}


/****************************************************************************
 * Human-readable device info.
 */
status_t __radeon_DeviceInfo( accelerant_device_info *adi )
{
	dprintf(("Radeon_accel: deviceinfo - ENTER\n"));
	
	if (adi->version >= 1)
	{
		adi->version = B_ACCELERANT_VERSION;
		strcpy( adi->name, "ATI Radeon based card");
		strcpy( adi->chipset, "ATI Radeon");
//		adi->memory = ci->ci_MemSize;
//		adi->dac_speed = CLOCK_MAX / 1000;
	}
	else
	{
		adi->version = B_ACCELERANT_VERSION;
	}
		
	dprintf(("Radeon_accel: deviceinfo - EXIT\n"));
	return (B_OK);
}

status_t Radeon_init_clone (void *data)
{
	status_t retval = B_OK;
	int32 ret;
	radeon_getglobals gg;
	char devname[B_OS_NAME_LENGTH + 15];

	dprintf (("radeon_accel: Radeon_init_clone() src=0x%08x\n", data));
	strcpy (devname, "/dev/graphics/");
	strcat (devname, (char *) data);
	
	dprintf (("radeon_accel: Radeon_init_clone() path = %s \n", devname ));

	if ((primDevFD = open (devname, B_READ_WRITE)) < 0)
	{
		dprintf (("radeon_accel: Radeon_init_clone() open failed = %x \n", devname ));
		return (B_ERROR);
	}

	gg.ProtocolVersion = RADEON_IOCTLPROTOCOL_VERSION;
	if ((retval = ioctl (primDevFD, RADEON_IOCTL_GETGLOBALS, &gg, sizeof (gg))) < 0)
	{
		dprintf (("Radeon_accel: init - Failed to get globals.\n"));
		return (retval);
	}

	if ((dataArea = clone_area ("Radeon accel driver data: share", (void **) &ci,
		  B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gg.GlobalArea)) < 0)
	{
		dprintf (("Radeon_accel: init - Failed to clone global area.\n"));
	}
dprintf(( "Radeon_accel cloned ci = %p \n", ci ));

	dprintf (("Radeon_accel: init - InitClient.\n"));
	primMemMgr = __mem_InitClient (primDevFD);
	dprintf (("Radeon_accel: MemMgr_2D %p\n", primMemMgr ));

	Radeon_CPInitClone( ci );
//	dprintf (("Radeon_accel: init - returning 0x%x\n", ci_areaid < 0 ? ci_areaid : B_OK));
	return B_OK;
}


