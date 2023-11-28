#ifndef _IDE_H
#define _IDE_H

#define IDE_MODULE_NAME "bus_managers/ide/v0.6"

#include <bus_manager.h>
#include <SupportDefs.h>
#include <lendian_bitfield.h>

#include <Drivers.h>

enum {
	B_GET_IDE_DEVICE_INFO = B_DEVICE_OP_CODES_END + 1,
	B_GET_IDE_DEVICE_STATUS
};

enum {
	B_IDE_DMA_STATUS_UNSUPPORTED,
	B_IDE_DMA_STATUS_ENABLED,
	B_IDE_DMA_STATUS_USER_DISABLED_CONFIG,
	B_IDE_DMA_STATUS_USER_DISABLED_SAFE_MODE,
	B_IDE_DMA_STATUS_CONTROLLER_DISABLED,
	B_IDE_DMA_STATUS_DRIVER_DISABLED_CONFIG,
	B_IDE_DMA_STATUS_DRIVER_DISABLED_FAILURE
};

typedef struct {
	uint8	version;	// 1
	uint8	dma_status;
	uint8	pio_mode;
	uint8	dma_mode;
} ide_device_status;

enum {
	IDE_CHS_MODE = 5,
	IDE_LBA_MODE = 7
};

#define ATA_BSY		0x80
#define ATA_DRDY	0x40
#define ATA_DRQ		0x08
#define ATA_ERR		0x01

typedef union {
	struct {
		uint8	features;
		uint8	sector_count;
		uint8	sector_number;
		uint8	cylinder_0_to_7;
		uint8	cylinder_8_to_15;
		LBITFIELD8_3(
			head					: 4,	// bit 0-3
			device					: 1,	// bit 4
			mode					: 3		// bit 5-7
				// 5 for chs
		);
		uint8	command;
	} chs;
	struct {
		uint8	features;
		uint8	sector_count;
		uint8	lba_0_to_7;
		uint8	lba_8_to_15;
		uint8	lba_16_to_23;
		LBITFIELD8_3(
			lba_24_to_27			: 4,	// bit 0-3
			device					: 1,	// bit 4
			mode					: 3		// bit 5-7
				// 7 for lba
		);
		uint8	command;
	} lba;
	struct {
		LBITFIELD8_3(
			dma						: 1,	// bit 0
			ovl						: 1,	// bit 1
			na1						: 6		// bit 2-7
		);
		LBITFIELD8_2(
			na2						: 3, // bit 0-2
			tag						: 5  // bits 3-7
		);
		uint8	reserved;
		uint8	byte_count_0_to_7;
		uint8	byte_count_8_to_15;
		LBITFIELD8_5(
			na3						: 4,	// bit 0-3
			device					: 1,	// bit 4
			obs1					: 1,	// bit 5
			na4						: 1,	// bit 6
			obs2					: 1		// bit 7						
		);
		uint8	command;	// 0xa0 for atapi
	} atapi_input; // sent to device
	struct {
		LBITFIELD8_5(
			ili						: 1,	// bit 0
			eom						: 1,	// bit 1
			abrt					: 1,	// bit 2
			na1						: 1, 	// bit 3
			sense					: 4		// bits 4-7 
		);
		LBITFIELD8_4(
			cmd_or_data				: 1, // bit 0  (1 == cmd, 0 == data)
			input_or_output		 	: 1, // bit 1  
			release					: 1, // bit 2		 
			tag						: 5  // bits 3-7
		);
		uint8	byte_count_0_to_7;
		uint8	byte_count_8_to_15;
		LBITFIELD8_5(
			na3						: 4,	// bit 0-3
			device					: 1,	// bit 4
			obs1					: 1,	// bit 5
			na4						: 1,	// bit 6
			obs2					: 1		// bit 7						
		);
		LBITFIELD8_7(
			chk						: 1,	// bit 0
			na2		 				: 2, 	// bit 1-2
			drq						: 1,	// bit 3
			serv					: 1,	// bit 4
			dmrd					: 1, 	// bit 5		 
			drdy					: 1,	// bit 6
			bsy						: 1		// bit 7						
		);
	} atapi_output; // received from device
#if 0
	struct {
		uint8	r1;		// features / error
		uint8	r2;		// sector count
		uint8	r3;		// sector number
		uint8	r4;		// cyl low
		uint8	r5;		// cyl high
		uint8	r6;		// device/head
		uint8	r7;		// command / status
	} raw;
#endif
	struct {
		uint8	r[7];
	} raw;
	struct {
		uint8	features;
		uint8	sector_count;
		uint8	sector_number;
		uint8	cylinder_low;
		uint8	cylinder_high;
		uint8	device_head;
		uint8	command;
	} write;
	struct {
		uint8	error;
		uint8	sector_count;
		uint8	sector_number;
		uint8	cylinder_low;
		uint8	cylinder_high;
		uint8	device_head;
		uint8	status;
	} read;
} ide_task_file;

//		uint16	reg01000b;	// data
//		uint8	reg10110b;	// alternate status / device control

typedef enum {
	ide_mask_features	 	= 0x01,
	ide_mask_error			= 0x01,
	ide_mask_sector_count	= 0x02,
	ide_mask_sector_number	= 0x04,
	ide_mask_cylinder_low	= 0x08,
	ide_mask_cylinder_high	= 0x10,
	ide_mask_cylinder		= 0x18,
	ide_mask_device_head	= 0x20,
	ide_mask_command		= 0x40,
	ide_mask_status			= 0x40,

	ide_mask_all			= 0x7f


} ide_reg_mask;

typedef struct {
	bus_manager_info	binfo;
	uint32 (*get_nth_cookie) (uint32 bus);
	uint32 (*get_bus_count) ();
	int32 (*get_abs_bus_num) (uint32 cookie);
		// 0 primary, 1 secondary, -1 invisible
	
	status_t (*acquire_bus) (uint32 cookie);
	status_t (*release_bus) (uint32 cookie);
	
	status_t (*write_command_block_regs)
		(uint32 cookie, ide_task_file *tf, ide_reg_mask mask);
	status_t (*read_command_block_regs)
		(uint32 cookie, ide_task_file *tf, ide_reg_mask mask);

	uint8 (*get_altstatus) (uint32 cookie);
	void (*write_device_control) (uint32 cookie, uint8 val);	

	void (*write_pio_16) (uint32 cookie, uint16 *data, uint16 count);
	void (*read_pio_16) (uint32 cookie, uint16 *data, uint16 count);

	status_t (*intwait) (uint32 cookie, bigtime_t timeout);

	/* return B_NOT_ALLOWED if controller does not support dma */
	status_t (*prepare_dma)(uint32 cookie, const iovec *vec, size_t count,
	                        uint32 startbyte, uint32 blocksize,
	                        size_t *numBytes, bool to_device);
	status_t (*start_dma)(uint32 cookie);
	status_t (*finish_dma)(uint32 cookie);
	uint32 (*get_bad_alignment_mask) (uint32 cookie);

	status_t (*get_dma_mode)(uint32 cookie, bool device1, uint32 *mode);
	status_t (*set_dma_mode)(uint32 cookie, bool device1, uint32 mode);
} ide_bus_info;

typedef struct {
	bus_manager_info	binfo;
	status_t (*get_nth_ide_bus) (uint32 bus, ide_bus_info **ide_bus, uint32 *cookie);
	uint32 (*get_ide_bus_count) ();
	
	status_t (*handle_device) (uint32 busnum, bool device1);
	status_t (*release_device) (uint32 busnum, bool device1);
	// protocol
	status_t (*wait_status)(ide_bus_info *bus, uint32 cookie,
		status_t (*predicate)(uint8 status, bigtime_t start_time,
	                      bigtime_t sample_time, bigtime_t timeout),
		bigtime_t timeout);
	status_t (*send_ata) (ide_bus_info *ide_bus, uint32 cookie, ide_task_file *tf,
						  bool drdy_required);
	status_t (*reset) (ide_bus_info *ide_bus, uint32 cookie);
} ide_module_info;

#endif /* _IDE_H */
