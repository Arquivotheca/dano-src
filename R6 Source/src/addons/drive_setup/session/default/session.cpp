//--------------------------------------------------------------------
//	
//	session.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <scsi.h>

#include <drive_setup.h>


//====================================================================

status_t ds_get_nth_session(int32 dev, int32 index, int32 block_size,
							session_data *session)
{
	uint64		start;
	status_t	result = B_ERROR;
	scsi_toc	toc;

	if ((index >= 0) && ((ioctl(dev, B_SCSI_GET_TOC, &toc) >= 0)) &&
		(index <= (toc.toc_data[3] - toc.toc_data[2]))) {

		start = ((toc.toc_data[4 + 5] * 60) +	// minutes
				 (toc.toc_data[4 + 6]) * 75) +	// seconds
				  toc.toc_data[4 + 7];			// frames

		session->data = (toc.toc_data[8 * index + 4 + 1] & 4);

		session->offset = (((toc.toc_data[8 * index + 4 + 5] * 60 * 75) +
						   (toc.toc_data[8 * index + 4 + 6] * 75) +
							toc.toc_data[8 * index + 4 + 7]) - start) *
							(2048 / block_size);

		session->blocks = ((((toc.toc_data[8 * (index + 1) + 4 + 5] * 60 * 75) +
							 (toc.toc_data[8 * (index + 1) + 4 + 6] * 75) +
							  toc.toc_data[8 * (index + 1) + 4 + 7]) - start) *
							  (2048 / block_size)) - session->offset;

		
		result = B_NO_ERROR;
	}
	return result;
}
