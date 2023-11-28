#if !defined(__DCTDECODE_H_)
#define __DCTDECODE_H_

#include "Pusher.h"
#include "jpeg6b.h"

namespace Pushers {

class DCTDecode : public Pusher, public JPEGDecompressor {
private:
uint32				fBytesPerRow;
bool				fImageComplete;
#if DEBUG > 0
uint32				fScanLine;
#endif

protected:

virtual status_t	ProcessHeader(uint width, uint height, uint bytesPerPixel);
virtual status_t	ProcessScanLine(const uint8 *buffer, uint rows);
virtual void		ImageComplete(void);

public:

					DCTDecode(Pusher *sink);
virtual				~DCTDecode();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);


};

}; // namespace Pushers

using namespace Pushers;
#endif
