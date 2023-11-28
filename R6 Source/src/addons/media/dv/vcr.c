/* vcr.c
 *
 * Wrapper functions for making AV/C VCR subunit commands.
 *
 */

#include "avc.h"
#include "vcr.h"

int32
vcr_get_input_signal_mode(avc_subunit *su, uchar *signal_mode, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;

	c.command_type	= AVC_STATUS;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_INPUT_SIGNAL_MODE;
	c.operand[0]	= 0xff;
	
	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	*signal_mode = r.operand[0];

	return r.response_code;
}

int32
vcr_get_output_signal_mode(avc_subunit *su, uchar *signal_mode, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;

	c.command_type	= AVC_STATUS;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_OUTPUT_SIGNAL_MODE;
	c.operand[0]	= 0xff;
	
	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	*signal_mode = r.operand[0];

	return r.response_code;
}

int32
vcr_record(avc_subunit *su, uchar record_mode, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;
	
	c.command_type	= AVC_CONTROL;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_RECORD;
	c.operand[0]	= record_mode;
	
	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	return r.response_code;
}

int32
vcr_play(avc_subunit *su, uchar play_mode, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;
	
	c.command_type	= AVC_CONTROL;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_PLAY;
	c.operand[0]	= play_mode;
	
	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	return r.response_code;
}

#define bcd2dec(x)		(((x)>>4)*10+((x)&0xf))

int32
vcr_get_time_code(avc_subunit *su, vcr_time_code *time, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;

	c.command_type	= AVC_STATUS;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_TIME_CODE;
	c.operand[0]	= 0x71;
	c.operand[1]	= 0xff;
	c.operand[2]	= 0xff;
	c.operand[3]	= 0xff;
	c.operand[4]	= 0xff;

	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 5, timeout)) < B_OK)
		return err;

	if (r.operand[0] != 0x71)
		return B_ERROR;  // ?

	time->hours		= bcd2dec(r.operand[4]);
	time->minutes	= bcd2dec(r.operand[3]);
	time->seconds	= bcd2dec(r.operand[2]);

	// 0x7f means the device does not support frames
	if (r.operand[1] == 0x7f)
		time->frames = 0;
	else
		time->frames = bcd2dec(r.operand[1]);

	return r.response_code;
}

int32
vcr_transport_state(avc_subunit *su, uchar *mode, uchar *state, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;
	
	c.command_type	= AVC_STATUS;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_TRANSPORT_STATE;
	c.operand[0]	= 0x7f;
	
	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	*mode = r.opcode;
	*state = r.operand[0];
	
	return r.response_code;
}

int32
vcr_wind(avc_subunit *su, uchar wind_mode, bigtime_t timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;
	
	c.command_type	= AVC_CONTROL;
	c.subunit_type	= su->type;
	c.subunit_id	= su->id;
	c.opcode		= VCR_WIND;
	c.operand[0]	= wind_mode;
	
	if ((err = send_avc_command(su->fd, su->guid, &c, &r, 1, timeout)) < B_OK)
		return err;
	
	return r.response_code;
}

