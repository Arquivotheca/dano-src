#if !defined(__LZWDECODE_H__)
#define __LZWDECODE_H__

#include "Pusher.h"

namespace Pushers {

class LZWDecode : public Pusher {

typedef struct {
	int16	last_code;
	uint8	this_code;
	uint8	stack;
} dict_entry;

void				PrivateReset(void);
void				ResetTable(void);
uint8				FirstCode(int16 start_code);
ssize_t				ChaseCodes(int16 start_code, bool finish);
void				AddCode(int16 last_code, uint8 this_code);
uint32				fBitBuffer;
uint32				fBufferedBits;
dict_entry			*fTable;
int32				fStackTop;
uint16				fMaxCode;
uint16				fCodeBits;
uint16				fCodeMask;
int16				fLastCode;
uint32				fEarlyChange;

public:
					LZWDecode(Pusher *sink, uint32 earlyChange);
					~LZWDecode();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);
};

}; // namespace Pushers

using namespace Pushers;

#endif
