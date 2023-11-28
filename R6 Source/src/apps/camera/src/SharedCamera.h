#ifndef SHAREDCAM_H
#define SHAREDCAM_H

#include "List.h"

enum {
	CAM_PROBE		= 'prob',
	CAM_QUERY		= 'qury',
	CAM_DELETE		= 'dlet',
	CAM_SAVE		= 'save',
	CAM_THUMB		= 'thmb',
};

class FilenameList : public BList {
public:
	FilenameList();
	~FilenameList();
	void MakeEmpty();
};

uint8 calculate_checksum(uint8 *buf, int start_pos, int stop_pos);
int YCCtoRGB32(uint8 *inbuf, uint8 *outbuf, uint32 inlen);


#endif
