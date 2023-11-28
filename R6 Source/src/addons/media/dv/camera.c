#include "avc.h"
#include "camera.h"

status_t camera_zoom(
				avc_subunit		*subunit,
				uchar			zoom_mode,
				bigtime_t		timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;

	c.command_type	= AVC_CONTROL;
	c.subunit_type	= subunit->type;
	c.subunit_id	= subunit->id;
	c.opcode		= CAMERA_ZOOM;
	c.operand[0]    = zoom_mode;

	if ((err = send_avc_command(subunit->fd, subunit->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	return r.response_code;
}


status_t camera_get_zoom(
				avc_subunit		*subunit,
				uchar			*zoom_mode,
				bigtime_t		timeout)
{
	avc_command_frame c;
	avc_response_frame r;
	status_t err;

	c.command_type	= AVC_STATUS;
	c.subunit_type	= subunit->type;
	c.subunit_id	= subunit->id;
	c.opcode		= CAMERA_ZOOM;
	c.operand[0]    = 0xff;

	if ((err = send_avc_command(subunit->fd, subunit->guid, &c, &r, 1, timeout)) < B_OK)
		return err;

	*zoom_mode = r.operand[0];

	return r.response_code;
}
