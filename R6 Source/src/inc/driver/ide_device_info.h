#ifndef IDE_DEVICE_INFO_H
#define IDE_DEVICE_INFO_H

#include <BeBuild.h>
#include <lendian_bitfield.h>

#define IDE_PROTOCOL_IS_ATA(di) (di->general_configuration.protocol_type < 2)
#define IDE_PROTOCOL_IS_ATAPI(di) (di->general_configuration.protocol_type == 2)

#if __MWERKS__
#pragma options align=packed
#endif

typedef struct {
	// ATA & ATAPI
	struct {
		LBITFIELD7(
		// ATAPI only
			command_packet_size					: 2,	// bit 0-1
				// 00b: 12 bytes (all CD-ROM)
				// 01b: 16 bytes (SAM???)
				// 1?b: reserved
			reserved_4_3_2						: 3,	// bit 2-4
			CMD_DRQ_type						: 2,	// bit 5-6
				// 00b: Microprocessor DRQ
				//      poll for 3ms
				// 01b: Interupt DRQ
				//      wait 10ms for interrupt
				// 10b: Accelerated DRQ
				//      poll for 50us
				// 11b: reserved
				
				// For ATA bit 6 is defined as
				// "not removable controller and/or device"
				// We do not use this.
				// Some ATA devices (SyQuest SyJet)
				// even fill in the ATAPI bits
		// ATA & ATAPI
			removable_media						: 1,	// bit 7
		// ATAPI only
			device_type							: 5,	// bit 8-12
				// 00h: Direct-access device
				// 01h:	Sequential-access device
				// 02h:	Printer device
				// 03h: Processor device
				// 04h: Write-once device
				// 05h: CD-ROM device
				// 06h: Scanner device
				// 07h: Optical memory device
				// 08h: Medium changer device
				// 09h: Communications device
				// 0Ch: Array controller device
				// 1Fh: Unknown or no device type
			reserved_13							: 1,	// bit 13
		// ATA & ATAPI
			protocol_type						: 2		// bit 14-15
				// 0?b: ATA
				// 10b: ATAPI
				// 11b: reserved
		);
	} general_configuration;

	// ATA only
	// Logical CHS
	uint16	default_cylinders;
	uint16	reserved_2;
	uint16	default_heads;
	uint16	unformatted_bytes_per_track;				// retired
	uint16	unformatted_bytes_per_sector;				// retired
	uint16	default_sectors_per_track;
	uint16	reserved_7_to_9[3];

	// ATA & ATAPI optional
	char	serial_number[20];

	// none
	uint16	vendor_specific_20;
	uint16	vendor_specific_21;
	uint16	ecc_bytes_avalable;							// obsolete

	// ATA & ATAPI
	char	firmware_revision[8];
	char	model_number[40];

	// ATA only
	struct {
		LBITFIELD2(
			maximum_sectors_per_interupt		: 8,	// bit 0-7
			vendor_specific_8_to_15				: 8		// bit 8-15
		);
	} multiple_sector_command_support;
	uint16	reserved_48;

	// ATA & ATAPI
	struct {
		LBITFIELD9(
			vendor_unique						: 8,	// bit 0-7
			dma_supported						: 1,	// bit 8
			lba_supported						: 1,	// bit 9
			iordy_can_disable					: 1,	// bit 10
			iordy_supported						: 1,	// bit 11
			// 0: iordy may be supported
			// 1: iordy is supported
		// ATAPI
			software_reset_required				: 1,	// bit 12
			// some old drives will not respond to
			// DEVICE_RESET when BSY is set to 1
		
		// ATAPI -- standby timer for ATA
			overlap_operation_supported			: 1,	// bit 13

		// ATAPI
			command_queuing_supported			: 1,	// bit 14
			interleaved_dma_supported			: 1		// bit 15
		);
	} capabilities;

	// ATA only
	struct {
		LBITFIELD3(
			standby_timer_value_valid			: 1,	// bit 0
			reserved_1_to_13					: 13,	// bit 1-13
			capabilities_valid					: 2		// bit 14-15
				// 01b: capabilities valid for ATA
		);
	} capabilities_50;

	// ATA & ATAPI
	struct {
		LBITFIELD2(
			vendor_specific_0_to_7				: 8,	// bit 0-7
			pio_data_transfer_mode_number		: 8		// bit 8-15
		);
	} pio_cycle_timing;
	uint16	reserverd_52;
	//uint16	dma_cycle_timing; // word 52

	struct {
		LBITFIELD4(
			word_54_to_58_valid					: 1,	// bit 0
				// 0: may be valid
				// 1: valid
			word_64_to_70_valid					: 1,	// bit 1
				// 0: not valid
				// 1: valid
			word_88_valid						: 1,	// bit 2
				// 0: not valid
				// 1: valid
			reserved_2_to_15					: 13	// bit 3-15
		);
	} field_validity;	// word 53

	// ATA only
	// always valid if field_validity.word_54_to_58_valid == 1
	// may be valid if field_validity.word_54_to_58_valid == 0
	// Current logical CHS
	uint16	current_cylinders;
	uint16	current_heads;
	uint16	current_sectors_per_track;
	uint32	current_capacity_in_sectors;

	// ATA only
	struct {
		LBITFIELD3(
			current_sectors_per_interupt		: 8,	// bit 0-7
			setting_valid						: 1,	// bit 8
			vendor_specific_9_to_15				: 7		// bit 9-15
		);
	} multiple_sector_command_setting;
	uint32	user_addressable_sectors;

	// ATA & ATAPI
	struct {
		LBITFIELD2(
			supported_modes						: 8,	// bit 0-7
			selected_mode						: 8		// bit 8-15
		);
	} singleword_dma_mode;	// retired for ATA-3
	struct {
		LBITFIELD2(
			supported_modes						: 8,	// bit 0-7
			selected_mode						: 8		// bit 8-15
		);
	} multiword_dma_mode;

	// ATA & ATAPI
	// valid when bit 1 of word 53 is one
	// (field_validity.word_64_to_70_valid == 1)
	struct {
		LBITFIELD2(
			supported_modes						: 8,	// bit 0-7
			reserved_8_to_15					: 8		// bit 8-15
		);
	} enhanced_pio_mode;

	// cycle time in nanoseconds
	uint16	minimum_multi_word_dma_cycle_time;

	// optional ATA & ATAPI
	// valid when bit 1 of word 53 is one
	// (field_validity.word_64_to_70_valid == 1)
	uint16	recomended_multi_word_dma_cycle_time;
	uint16	minimum_pio_cycle_time_without_flow_control;
	uint16	minimum_pio_cycle_time_with_iordy_flow_control;
	
	// none
	uint16	reserved_69;	// reserved for advanced
	uint16	reserved_70;	// PIO support
	
	// optional ATAPI
	// typical release times in microseconds for overlap and queing commands
	uint16	release_time_after_command_received;
	uint16	release_time_after_service;
	
	uint16	major_version_atapi;	// version 0x0000 or 0xffff for
	uint16	minor_version_atapi;	// unknown version

	// ATA only
	struct {
		LBITFIELD2(
			maximum_queue_depth					: 5,	// bit 0-4
			reserved_5_to_15					: 11	// bit 5-15
		);
	} queue_depth;

	uint16	reserved_76_to_79[4];

	// ATA/ATAPI-3 up only
	uint16	major_version;	// version 0x0000 or 0xffff for
	uint16	minor_version;	// unknown version
	struct {
		LBITFIELD16(
			smart								: 1,	// bit 0
			security_mode						: 1,	// bit 1
			removable_media						: 1,	// bit 2
			power_management					: 1,	// bit 3
			packet_command						: 1,	// bit 4
			write_cache							: 1,	// bit 5
			look_ahead							: 1,	// bit 6
			release_interrupt					: 1,	// bit 7
			service_interrupt					: 1,	// bit 8
			device_reset_command				: 1,	// bit 9
			host_protected_area					: 1,	// bit 10
			reserved_11							: 1,	// bit 11
			write_buffer_command				: 1,	// bit 12
			read_buffer_command					: 1,	// bit 13
			nop_command							: 1,	// bit 14
			reserved_15							: 1		// bit 15
		);
		LBITFIELD7(
			download_microcode_command			: 1,	// bit 0
			read_write_dma_queued				: 1,	// bit 1
			cfa									: 1,	// bit 2
			advanced_power_management			: 1,	// bit 3
			removable_media_status_notification	: 1,	// bit 4
			reserved_5_to_13					: 9,	// bit 5-13
			valid								: 2		// bit 14-15
				// 01b: command_set_supported
				//      is valid
		);
	} command_set_feature_supported;
	uint16	command_set_supported_extention;
	struct {
		LBITFIELD16(
			smart								: 1,	// bit 0
			security_mode						: 1,	// bit 1
			removable_media						: 1,	// bit 2
			power_management					: 1,	// bit 3
			packet_command						: 1,	// bit 4
			write_cache							: 1,	// bit 5
			look_ahead							: 1,	// bit 6
			release_interrupt					: 1,	// bit 7
			service_interrupt					: 1,	// bit 8
			device_reset_command				: 1,	// bit 9
			host_protected_area					: 1,	// bit 10
			reserved_11							: 1,	// bit 11
			write_buffer_command				: 1,	// bit 12
			read_buffer_command					: 1,	// bit 13
			nop_command							: 1,	// bit 14
			reserved_15							: 1		// bit 15
		);
		LBITFIELD6(
			download_microcode_command			: 1,	// bit 0
			read_write_dma_queued				: 1,	// bit 1
			cfa									: 1,	// bit 2
			advanced_power_management			: 1,	// bit 3
			removable_media_status_notification	: 1,	// bit 4
			reserved_5_to_13					: 11	// bit 5-15
		);
	} command_set_feature_enabled;
	uint16	command_set_feature_default;

	struct {
		LBITFIELD4(
			supported_modes						: 7,	// bit 0-6
			reserved_5_to_7						: 1,	// bit 7
			selected_mode						: 7,	// bit 8-14
			reserved_13_to_15					: 1		// bit 15
		);
	} ultra_dma_mode;	// word 88

	uint16	time_required_for_security_erase_unit_completion;
	uint16	time_required_for_enhanced_security_erase_unit_completion;
	struct {
		LBITFIELD2(
			current_level						: 8,	// bit 0-7
			reserved_8_to_15					: 8		// bit 8-15
		);
	} advanced_power_management_level_value;
	uint16	reserved_92_to_125[34];
	struct {
		LBITFIELD2(
			last_lun							: 3,	// bit 0-2
			reserved_3_to_15					: 13	// bit 3-15
		);
	} last_lun_word;
	struct {
		LBITFIELD4(
			media_status_notification_supported	: 2,	// bit 0-1
			// 00b: not supported
			// 01b: supported
			// 1?b: reserved
			reserved_2_to_7						: 6,	// bit 2-7
			device_write_protect				: 1,	// bit 8
			reserved_9_to_15					: 7		// bit 9-15
		);
	} removable_media;
	struct {
		LBITFIELD9(
			security_supported					: 1,	// bit 0
			security_enabled					: 1,	// bit 1
			security_locked						: 1,	// bit 2
			security_frozen						: 1,	// bit 3
			security_count_expired				: 1,	// bit 4
			enhanced_security_erase_supported	: 1,	// bit 5
			reserved_6_to_7						: 2,	// bit 6-7
			security_level						: 1,	// bit 8
				// 0: high
				// 1: maximum
			reserved_9_to_15					: 7		// bit 9-15
		);
	} security_status;
	uint16	vendor_specific_129_to_159[31];
	uint16	reserved_160_to_255[96];
} _PACKED ide_device_info;

#if __MWERKS__
#pragma options align=reset
#endif



/* swap_ide_device_info:
** convert ide_device_info buffer between raw device format
** and native host format
*/
void prepare_ide_device_info(ide_device_info *dc);
void print_ide_device_info(void (*ide_dprintf)(), ide_device_info *di);
status_t get_selected_dma_mode(ide_device_info *di, uint32 *modep);

#endif /* IDE_DEVICE_INFO_H */
