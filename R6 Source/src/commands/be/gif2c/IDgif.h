#ifndef IDGIF_H
#define IDGIF_H

#include "gfxcodec.h"

extern "C" {
_EXPORT char *rrasaddon_IDName();
_EXPORT char *rrasaddon_IDAuthor();
_EXPORT char *rrasaddon_IDNotice();
_EXPORT char *rrasaddon_IDEncoder();
_EXPORT char *rrasaddon_IDDecoder();
_EXPORT float CanCreateImage(void *bytes, long byteLen);
_EXPORT GfxImage *CreateImage(char *file);
}

#endif
