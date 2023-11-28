#if !defined(__TEEPUSHER_H_)
#define __TEEPUSHER_H_

#include "Pusher.h"
#include <stdio.h>

namespace Pushers {

class TeePusher : public Pusher {

FILE				*fOtherSink;
bool				fCloseFile;

public:
					TeePusher(Pusher *sink, FILE *othersink=0);
					TeePusher(Pusher *sink, char const *name);
virtual				~TeePusher();
void				SetFile(FILE *newsink);
void				SetFile(char const *name);
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
