/* jpeg_interface.h */

#ifndef __JPEG_INTERFACE_H
#define __JPEG_INTERFACE_H

#include <SupportDefs.h>

struct jpeg_stream {

	uint8		**linePtrs;

};

void InitJPEG(	uint32		width,
				uint32		height,
				jpeg_stream	*stream);

void DecodeJPEG(	void 		*inData,
					void		*outData,
					uint32		width,
					uint32		height,
					size_t		inSize,
					jpeg_stream	*stream);

status_t EncodeJPEG(	void 		*inData,
						void		*outData,
						size_t		*outSize,
						uint32		width,
						uint32		height,
						int32		quality,
						jpeg_stream	*stream);
#endif
