#if !defined(__JPEG6B_H__)
#define __JPEG6B_H__

#include <SupportDefs.h>

struct jpeg_decompress_struct;
struct jpeg_source_mgr;

class JPEGDecompressor {

jpeg_decompress_struct	*fDecoder;
size_t					fSkipMore;
uint8					*fBuffer;
size_t					fBytesInBuffer;
uint8					*fScans[4];
bool					fStartDecompressor;

enum {
	BUFFER_SIZE = 2048
};
typedef int boolean;
protected:
					JPEGDecompressor(void);
virtual				~JPEGDecompressor();
status_t			WriteData(const uint8 *buffer, size_t bytes);

virtual status_t	ProcessHeader(uint width, uint heigh, uint bytesPerPixel);
virtual status_t	ProcessScanLine(const uint8 *buffer, uint rows);
virtual void		ImageComplete(void);
private:

static void			init_source(jpeg_decompress_struct *);
static boolean		fill_input_buffer(jpeg_decompress_struct *);
static void			skip_input_data(jpeg_decompress_struct * cinfo, long num_bytes);
static void			term_source(jpeg_decompress_struct * cinfo);
};

#endif
