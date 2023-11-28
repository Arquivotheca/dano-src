#include "Object2.h"
#include "Pusher.h"

extern status_t PushStream(PDFObject *stream, Pusher *sink);
extern status_t PushBuffer(PDFObject *dict, const uint8 *buffer, ssize_t length, Pusher *sink);
extern status_t	PushContent(PDFObject *obj, Pusher *sink);
