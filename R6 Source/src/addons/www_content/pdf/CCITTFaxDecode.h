#if !defined(__CCITTFAXDECODE_H_)
#define __CCITTFAXDECODE_H_

#include "Pusher.h"
#include "Object2.h"

namespace Pushers {

class CCITTFaxDecode : public Pusher {
struct fax_code {
	uint8	code_length;
	uint8	code_bits;
	uint16	run_length;
};
static const fax_code	white_runs[];
static const fax_code	black_runs[];
static const fax_code	*white_runs_starts[];
static const fax_code	*black_runs_starts[];
static const uint8		modes[8][4];

int32				m_K;				// K from decode parms
bool				m_EndOfLine;
bool				m_EncodedByteAlign;
int32				m_Columns;
int32				m_Rows;
bool				m_EndOfBlock;
bool				m_BlackIs1;
uint32				m_DamagedRowsBeforeError;

int32				m_a0;	// G3-2d and G4 encoding parms
int32				m_a1;
int32				m_a2;
int32				m_b1;
int32				m_b2;

int32				m_mode;
uint32				m_row_bytes;
uint8				*m_prev_line;
uint8				*m_this_line;
int32				m_run_length;		// current working run length
uint8				m_code_length;	// length in bits of the accumulated code
uint8				m_code_bits;		// actual bits in accumulated code
uint8				m_source_length;	// number of bits remaining in source bits
uint8				m_source_bits;	// actuall remaining source bits

#ifndef NDEBUG
uint8				m_last_code_bits;
uint8				m_last_code_length;
uint32				m_line_num;
#endif

bool				m_doing_black;

static int			fax_code_compare(uint8 const *key, fax_code const *item);
CCITTFaxDecode::fax_code const * 
					findPair(void);
bool				doOneDimBitRun(void);
void				InvertThisLine(void);
public:
					CCITTFaxDecode(Pusher *sink, PDFObject *parms);
virtual				~CCITTFaxDecode();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers

using namespace Pushers;

#endif
