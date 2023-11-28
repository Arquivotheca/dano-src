#if !defined(__OBJECTPARSER_H_)
#define __OBJECTPARSER_H_

#include "Pusher.h"
#include <StringBuffer.h>

namespace BPrivate {
class ObjectSink;

class ObjectParser : public Pusher {

enum lexer_type {
	LEX_UNKNOWN = 0,
	LEX_COMMENT,
	LEX_NAME,
	LEX_PAREN_STRING,
	LEX_HEX_STRING,
	LEX_KEYWORD,
	LEX_NUMBER
};

uint32				fNumber;
uint32				fHasDecimal;
ObjectSink			*fObjectSink;
StringBuffer		fString;
lexer_type			fType;
uint32				fParenDepth;
uint8				fDoingEscape;
uint8				fHexByte;
bool				fDoingRawWrites;
bool				fTokenFinished;
bool				fDone;
bool				fNegate;

					ObjectParser();
void				SniffTokenType(uint8 aByte);
bool				DoHex(uint8 aByte);
bool				ParenStringEscape(uint8 aByte);
ssize_t				WriteObjects(const uint8 *buffer, ssize_t length, bool finish);

public:
					ObjectParser(ObjectSink *sink);
virtual				~ObjectParser();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace BPrivate

using namespace BPrivate;

#endif
