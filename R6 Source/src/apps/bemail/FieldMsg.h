#ifndef FIELD_MSG_H
#define FIELD_MSG_H

enum {
	FIELD_CHANGED = 0x0800
};

enum {
	FIELD_TO = 0x01,
	FIELD_SUBJECT = 0x02,
	FIELD_CC = 0x04,
	FIELD_BCC = 0x08,
	FIELD_BODY = 0x10,
};

#endif
