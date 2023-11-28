#include "Filter.h"
#include <SupportDefs.h>

class ASCIIHexDecode : public BInputFilter {
public:
					ASCIIHexDecode(BInputFilter *source);
virtual				~ASCIIHexDecode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

private:
uint8				byte_buffer[1024];
ssize_t				next_byte;
ssize_t				bytes_in_buffer;
uint8				hex;
uint8				nibbles;
bool				end_of_stream;
};

class ASCII85Decode : public BInputFilter {
public:
					ASCII85Decode(BInputFilter *source);
virtual				~ASCII85Decode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

private:
ssize_t				ReadFive();

uint8				byte_buffer[1024];
ssize_t				next_byte;
ssize_t				bytes_in_buffer;
uint32				char_buffer;
int32				buffered_chars;
bool				end_of_stream;
};

class LZWDecode : public BInputFilter {
public:
					LZWDecode(BInputFilter *source, uint32 earlyChange = 1);
virtual				~LZWDecode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

private:
typedef struct {
	int16	last_code;
	uint8	this_code;
	uint8	stack;
} dict_entry;

int16				NextByte(void);
int16				NextCode(void);
void				PrivateReset(void);
void				ResetTable(void);
uint8				FirstCode(int16 start_code);
size_t				ChaseCodes(int16 start_code, uint8 *buffer, size_t bytes_max);
void				AddCode(int16 last_code, uint8 this_code);
uint8				byte_buffer[1024];
ssize_t				buffered_bytes;
ssize_t				bytes_in_buffer;
uint32				bit_buffer;
uint32				buffered_bits;
dict_entry			*table;
int32				stack_top;
uint16				max_code;
uint16				code_bits;
uint16				code_mask;
int16				last_code;
uint32				early_change;
};

class RunLengthDecode : public BInputFilter {
public:
					RunLengthDecode(BInputFilter *source);
virtual				~RunLengthDecode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

private:

uint16				rle_code;
uint8				rle_value;
bool				end_of_stream;
};

class CCITTFaxDecode : public BInputFilter {
public:
					CCITTFaxDecode(BInputFilter *source);
virtual				~CCITTFaxDecode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

};

class DCTDecode : public BInputFilter {
public:
					DCTDecode(BInputFilter *source);
virtual				~DCTDecode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

};

class FlateDecode : public BInputFilter {
public:
					FlateDecode(BInputFilter *source);
virtual				~FlateDecode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

private:
typedef struct private_data;
private_data		*m_pd;
};

class RC4Decode : public BInputFilter {
public:
					RC4Decode(BInputFilter *source, const uint8 *key /* ten bytes */);
virtual				~RC4Decode();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);
void				SetKey(const uint8 *new_key);	// must call Reset() after SetKey() to take effect

private:
typedef struct private_data;
private_data		*m_pd;
};

class PredictedDecode : public BInputFilter {
public:
					PredictedDecode(BInputFilter *source, uint32 predictor = 1, uint32 columns = 1, uint32 colors = 1, uint32 bitsPerComponent = 8);
virtual				~PredictedDecode();
virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

private:

enum {
	DEFAULT_PREDICTOR = 1,
	TIFF_PREDICTOR,
	PNG_FILTER_VALUE_NONE = 10,
	PNG_FILTER_VALUE_SUB,
	PNG_FILTER_VALUE_UP,
	PNG_FILTER_VALUE_AVG,
	PNG_FILTER_VALUE_PAETH,
	PNG_FILTER_VALUE_OPTIMUM
};

typedef	uint8 * uint8p;
uint32		m_predictor;
uint32		m_columns;
uint32		m_colors;
uint32		m_bitsPerComponent;
uint32		m_rowBytes;
uint32		m_bpp;
uint8p		m_prev_row;
uint8p		m_row;
uint32		m_bytes_in_buffer;

void 				process_filter_row(uint8 the_predictor);
void				tiff_process_row(void);
ssize_t				FillRow(uint8p a_row);
ssize_t				ReadPNGRow(void);
ssize_t				ReadTIFFRow(void);
};