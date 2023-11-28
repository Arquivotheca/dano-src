#if !defined(__PREDICTEDDECODE_H_)
#define __PREDICTEDDECODE_H_

#include "Pusher.h"

namespace Pushers {

class PredictedDecode : public Pusher {

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

public:

					PredictedDecode(Pusher *sink, uint32 predictor = 1, uint32 columns = 1, uint32 colors = 1, uint32 bitsPerComponent = 8);
virtual				~PredictedDecode();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers

using namespace Pushers;

#endif
