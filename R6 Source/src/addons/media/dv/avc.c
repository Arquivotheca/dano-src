/* avc.c
 *
 * Audio/Video Control for 1394.
 *
 */

#include "avc.h"
#include "ieee1394.h"

status_t
send_avc_command(
		int fd,
		uint64 guid,
		avc_command_frame *cmd,
		avc_response_frame *rsp,
		size_t operand_count,
		bigtime_t timeout)
{
	status_t err;
	
	cmd->_zero_ = 0;
	err = send_fcp_command(
			fd, guid, cmd, operand_count + 3, rsp, operand_count + 3, timeout);
	
	return err;
}
