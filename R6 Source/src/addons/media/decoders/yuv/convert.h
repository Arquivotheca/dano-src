#ifndef CONVERT_H
#define CONVERT_H

#include <GraphicsDefs.h>
#include <MediaDefs.h>

typedef void (convertf)(uint32 width, uint32 height,
                        const void *src, uint32 src_bytes_per_row,
                        void *dest, uint32 dest_bytes_per_row);

extern convertf *get_converter(uint32 srctype, int *src_bytes_per_row,
                               media_video_display_info *display);

#endif
