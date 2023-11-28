#include <graphics_p/video_overlay.h>

#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/banshee/banshee_regs.h>
#include <graphics_p/3dfx/common/bena4.h>
#include <graphics_p/3dfx/common/debug.h>

#include <unistd.h>

#include <Entry.h>

#include "protos.h"
#include "thdfxI2C.h"

#define BT869_I2C_WR_ADDR		0x88		// 0x88 is the primary address, 0x8A is the alternate
#define BT869_I2C_RD_ADDR		0x89		// 0x89 is the primary address, 0x8B is the alternate
#define BT869_CONFIG_REG	0xB8

/****************************************************************************
 * Globals
 */
extern thdfx_card_info	*ci;
extern int		devfd;

thdfxI2C *gI2CBus;

void construct_i2c(int fd)
{
	gI2CBus = new thdfxI2C("3DFX Voodoo 3", fd);
//dprintf(("3dfx_accel: construct_i2c gI2CBus = 0x%x\n", gI2CBus));
}

void destroy_i2c()
{
	delete gI2CBus;
}

void enable_tvout(int enable)
{
	uint32 LTemp;
	bool	bt897_busy = true;
	int i2c_val;
	BEntry settings_entry("/boot/home/config/settings/voodoo3_tvout");
//dprintf(("3dfx_accel: enable_tvout - ENTER, enable = %d\n", enable));
	// Check to see whether the 3Dfx TV Out Settings file exists
	if (!settings_entry.Exists()) {
//dprintf(("3dfx_accel: enable_tvout - Settings file does not exist returning\n"));
		return;
	}

	ioctl(devfd, THDFX_IOCTL_ENABLE_TVOUT, &enable, sizeof (enable));
	
	if (!enable)		// disable the TVOut BEFORE we change the settings of the VGA controller
	{
//dprintf(("3dfx_accel: enable_tvout - resetting Bt897 & Disabling Video Out\n"));
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xBA, 0x80);			// Reset the Bt897
		while (bt897_busy)
		{
			gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xC4, 0x80);			// Write 0x80 to the ESTATUS bits so we can determine busy or not
			i2c_val = gI2CBus->I2CRead(BT869_I2C_RD_ADDR);					// Read the status.
			bt897_busy = (i2c_val & 0x01);
		}
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xc4, 0x00);		// Disable Video
	}

	LTemp = _V3_ReadReg( ci, V3_VID_IN_FORMAT );
	LTemp &= ~(SST_VIDEOIN_INTERFACE
							| SST_VIDEOIN_G4_FOR_POSEDGE
							| SST_VIDEOIN_GENLOCK_ENABLE
							| SST_VIDEOIN_NOT_USE_VGA_TIMING
							| BIT(18));								// clear all VideoIn format bits
	if (enable)
	{
		LTemp |= SST_VIDEOIN_TVOUT_ENABLE
							| SST_VIDEOIN_G4_FOR_POSEDGE
 						  | SST_VIDEOIN_GENLOCK_ENABLE
						  | SST_VIDEOIN_NOT_USE_VGA_TIMING
							| BIT(18) /* SST_VIDEOIN_GENLOCK_SOURCE_TV */;		// enable TV Out
	}
	_V3_WriteReg_NC( ci, V3_VID_IN_FORMAT, LTemp );
	
	LTemp = _V3_ReadReg( ci, V3_VID_SERIAL_PARALLEL_PORT );
	LTemp &= ~(SST_SERPAR_I2C_EN | SST_SERPAR_VMI_CS_N | BIT(31));	// clear all I2C Enable bits
	if (enable)
		LTemp |= SST_SERPAR_I2C_EN | SST_SERPAR_VMI_CS_N | BIT(31);
	_V3_WriteReg_NC( ci, V3_VID_SERIAL_PARALLEL_PORT, LTemp );
	
	if (enable)
	{
//dprintf(("3dfx_accel: enable_tvout - resetting Bt897 & Enabling Video Out\n"));
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xBA, 0x80);			// Reset the Bt897
		
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xB8, 0x00);			// Autoconfigure to mode 0 (NTSC 640x480)
		while (bt897_busy)
		{
			gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xC4, 0x80);			// Write 0x80 to the ESTATUS bits so we can determine busy or not
			i2c_val = gI2CBus->I2CRead(BT869_I2C_RD_ADDR);					// Read the status.
			bt897_busy = (i2c_val & 0x01);
		}
//dprintf(("3dfx_accel: enable_tvout - enabling colour bars\n"));
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0x6c, 0x80);		// Issue a Timing Reset
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xc4, 0x01);		// Enable Video
		gI2CBus->I2CWrite2(BT869_I2C_WR_ADDR, 0xba, 0x01);		// Disable DAC A (not used on Voodoo 3 3000)

#define HBLANK_BEGIN_CNT 90
#define VBLANK_BEGIN_CNT 74

		// Now we write the HBlank & VBlank counts - this ensures that the image is stable and positioned correctly on the screen
		LTemp = ( HBLANK_BEGIN_CNT << SST_TVOUT_HCOUNT_BLANK_BEGINS_SHIFT) | ((HBLANK_BEGIN_CNT+640) << SST_TVOUT_HCOUNT_BLANK_ENDS_SHIFT);		// Must be 640 pixels in the 'gap'
		_V3_WriteReg_NC( ci, V3_VID_TV_OUT_BLANK_H_COUNT, LTemp );
		
		LTemp = (VBLANK_BEGIN_CNT << SST_TVOUT_VCOUNT_BLANK_BEGINS_SHIFT) | ((VBLANK_BEGIN_CNT+480) << SST_TVOUT_VCOUNT_BLANK_ENDS_SHIFT);		// Must be 480 pixels in the 'gap'
		_V3_WriteReg_NC( ci, V3_VID_TV_OUT_BLANK_V_COUNT, LTemp );
	}
}
