/* avc.h
 *
 * AV/C definitions.
 *
 */

#ifndef AVC_H
#define AVC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SupportDefs.h>

/*
 * the high-level AV/C interface
 */
typedef struct avc_subunit {
	int		fd;								// fd of 1394 driver
	uint64	guid;							// GUID of AVC device
	uchar	type;					// subunit type
	uchar	id;						// subunit ID
} avc_subunit;

/*
 * the low-level AV/C interface
 */
#if __INTEL__
typedef struct {
	uchar command_type : 4;
	uchar _zero_ : 4;
	uchar subunit_id : 3;
	uchar subunit_type : 5;
	uchar opcode;
	uchar operand[512-3];
} _PACKED avc_command_frame;

typedef struct {
	uchar response_code : 4;
	uchar _zero_ : 4;
	uchar subunit_id : 3;
	uchar subunit_type : 5;
	uchar opcode;
	uchar operand[512-3];
} _PACKED avc_response_frame;
#else
#error bitfield layout must be defined for this processor
#endif /* __INTEL__ */

			
status_t send_avc_command(
			int fd,							// fd of 1394 driver
			uint64 guid,					// guid of device on the bus
			avc_command_frame *command,		// the command data
			avc_response_frame *response,	// the response data
			size_t operand_count,			// number of command operands
			bigtime_t timeout);				// timeout in microseconds

/* command types */
#define AVC_CONTROL					0x0
#define AVC_STATUS					0x1
#define AVC_SPECIFIC_INQUIRY		0x2
#define AVC_NOTIFY					0x3
#define AVC_GENERAL_INQUIRY			0x4

/* response codes */
#define AVC_NOT_IMPLEMENTED			0x8
#define AVC_ACCEPTED				0x9
#define AVC_REJECTED				0xa
#define AVC_IN_TRANSITION			0xb
#define AVC_IMPLEMENTED				0xc
#define AVC_CHANGED					0xd
#define AVC_INTERIM					0xf

/* subunit types */
#define AVC_VIDEO_MONITOR			0x0
#define AVC_DISC_PLAYER_RECORDER	0x3
#define AVC_TAPE_PLAYER_RECORDER	0x4
#define AVC_TUNER					0x5
#define AVC_VIDEO_CAMERA			0x7
#define AVC_VENDOR_UNIQUE			0x1c
#define AVC_SUBUNIT_TYPE_EXTENDED	0x1e
#define AVC_UNIT					0x1f

/* subunit IDs */
#define AVC_INSTANCE_0				0x0
#define AVC_INSTANCE_1				0x1
#define AVC_INSTANCE_2				0x2
#define AVC_INSTANCE_3				0x3
#define AVC_INSTANCE_4				0x4
#define AVC_SUBUNIT_ID_EXTENDED		0x5
#define AVC_ALL_INSTANCES			0x6
#define AVC_IGNORE					0x7

/*
 * command opcodes
 */
#define AVC_VENDOR_DEPENDENT			0x00
#define AVC_RESERVE						0x01
#define AVC_PLUG_INFO					0x02
#define AVC_OPEN_DESCRIPTOR				0x08
#define AVC_READ_DESCRIPTOR				0x09
#define AVC_WRITE_DESCRIPTOR			0x0a
#define AVC_SEARCH_DESCRIPTOR			0x0b
#define AVC_OBJECT_NUMBER_SELECT		0x0d
#define AVC_DIGITAL_OUTPUT				0x10
#define AVC_DIGITAL_INPUT				0x11
#define AVC_CHANNEL_USAGE				0x12
#define AVC_OUTPUT_PLUG_SIGNAL_FORMAT	0x18
#define AVC_INPUT_PLUG_SIGNAL_FORMAT	0x19
#define AVC_CONNECT_AV					0x20
#define AVC_DISCONNECT_AV				0x21
#define AVC_CONNECTIONS					0x22
#define AVC_CONNECT						0x24
#define AVC_DISCONNECT					0x25
#define AVC_UNIT_INFO					0x30
#define AVC_SUBUNIT_INFO				0x31
#define AVC_POWER						0xb2

#ifdef __cplusplus
}
#endif

#endif /* AVC_H */
