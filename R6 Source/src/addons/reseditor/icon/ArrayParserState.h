#ifndef ARRAY_PARSER_STATE_H
#define ARRAY_PARSER_STATE_H

#include "ArrayParser.h"

#include <DataIO.h>
#include <String.h>
#include <Path.h>

#include <stdio.h>
#include <vector>

namespace BPrivate {

class ArrayParserState
{
public:
	ArrayParserState(BArrayParser* parser);
	~ArrayParserState();
	
	status_t ReadArray(BPositionIO* output,
					   size_t* bytes_per_entry,
					   BString* identifier,
					   BMessage* meta_data);
	
	// Current parser state.
	const char* CurrentFile() const;
	int32 CurrentLine() const;
	
	// Error reporting.
	void Error(status_t code, const char *e, ...) const;
	void Error(const char *e, ...) const;
	void Warn(const char *e, ...) const;
	
private:
	BArrayParser* fParser;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
