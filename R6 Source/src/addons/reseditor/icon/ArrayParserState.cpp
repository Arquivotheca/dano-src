#include "ArrayParserState.h"
#include "array_scanner.h"

#include <File.h>
#include <Autolock.h>
#include <Debug.h>
#include <ByteOrder.h>

#include <errno.h>

extern int yylex( void* lvalp, ArrayParserState* pState );
extern FILE *yyin;
extern int yylineno;
extern int yydebug;

// ---------------------- ArrayParserState ----------------------

ArrayParserState::ArrayParserState(BArrayParser* parser)
	: fParser(parser)
{
}

ArrayParserState::~ArrayParserState()
{
}

static int32 convert_type(int token, int32 cur_type, bool* handled)
{
	if( handled ) *handled = true;
	
	switch( token ) {
		case CHAR:		return 1;
		case INT:		return cur_type > 0 ? cur_type : 2;
		case LONG:		return cur_type == 4 ? 8 : 4;
		case SIGNED:	return cur_type;
		case UNSIGNED:	return cur_type;
		case UCHAR:		return 1;
		case USHORT:	return 2;
		case ULONG:		return 4;
		case INT8:		return 1;
		case UINT8:		return 1;
		case INT16:		return 2;
		case UINT16:	return 2;
		case INT32:		return 4;
		case UINT32:	return 4;
		case INT64:		return 8;
		case UINT64:	return 8;
	}
	
	if( handled ) *handled = false;
	return cur_type;
}

static void parse_metadata(BMessage* into, const BString& ident, int64 value)
{
	if( ident.IFindFirst("width") >= 0 ) {
		into->RemoveName("width");
		into->AddInt32("width", (int32)value);
	} else if( ident.IFindFirst("height") ) {
		into->RemoveName("height");
		into->AddInt32("height", (int32)value);
	}
}

enum parse_state {
	P_START,
	P_DEFINE,
	P_TYPE,
	P_VALUE,
	P_INBRACKET,
	P_OUTBRACKET,
	P_DATA
};

status_t ArrayParserState::ReadArray(BPositionIO* output,
									 size_t* bytes_per_entry,
									 BString* identifier,
									 BMessage* meta_data)
{
	parse_state state = P_START;
	int32 type_size = 0;
	
	if( bytes_per_entry ) *bytes_per_entry = 0;
	if( output ) output->SetSize(0);
	
	for(;;) {
		ArrayParserValue value;
		int token = yylex(&value, this);
		if( token == EOS || token == 0 ) return B_ERROR;
		else if( token == ';' ) {
			// Semicolon always resets the parse state.
			state = P_START;
			type_size = 0;
			continue;
		}
		
		switch( state ) {
			case P_START: {
				if( token == DEFINE ) {
					state = P_DEFINE;
				} else {
					bool handled = false;
					type_size = convert_type(token, type_size, &handled);
					if( handled ) {
						if( identifier ) *identifier = "";
						if( output ) output->SetSize(0);
						state = P_TYPE;
					}
				}
			} break;
			case P_DEFINE: {
				if( token == IDENTIFIER ) {
					if( identifier ) *identifier = value.String();
				} else if( token == NUMBER ) {
					parse_metadata(meta_data, *identifier, value.Number());
					state = P_START;
					type_size = 0;
				}
			} break;
			case P_TYPE: {
				if( token == IDENTIFIER ) {
					if( identifier ) *identifier = value.String();
				} else if( token == NUMBER ) {
					parse_metadata(meta_data, *identifier, value.Number());
					state = P_START;
				} else if( token == '[' ) {
					state = P_INBRACKET;
				} else if( token == '=' ) {
					// Ignore.
				} else {
					bool handled = false;
					type_size = convert_type(token, type_size, &handled);
					if( !handled ) state = P_START;
				}
			} break;
			case P_INBRACKET: {
				// skip through everything that appears within [ ].
				if( token == ']' ) state = P_OUTBRACKET;
			} break;
			case P_OUTBRACKET: {
				// skip everything until the start of the array data.
				if( token == '{' ) state = P_DATA;
			} break;
			case P_DATA: {
				if( token == '}' ) {
					// found all the data!
					if( bytes_per_entry ) *bytes_per_entry = type_size;
					return B_OK;
				} else if( token == NUMBER ) {
					// append a value to the array
					if( output ) {
						switch( type_size ) {
							case 1: {
								uint8 v = (uint8)value.Number();
								output->Write(&v, sizeof(v));
							} break;
							case 2: {
								uint16 v = (uint16)value.Number();
								output->Write(&v, sizeof(v));
							} break;
							case 4: {
								uint32 v = (uint32)value.Number();
								output->Write(&v, sizeof(v));
							} break;
							case 5: {
								uint64 v = (uint64)value.Number();
								output->Write(&v, sizeof(v));
							} break;
						}
					}
				}
				// all other tokens are ignored.
			} break;
			default:
				debugger("Bad parser state");
				break;
		}
	}
	
	return B_ERROR;
}

const char* ArrayParserState::CurrentFile() const
{
	return "";
}

int32 ArrayParserState::CurrentLine() const
{
	return yylineno;
}

void ArrayParserState::Error(status_t code, const char *e, ...) const
{
	va_list vl;
	va_start(vl, e);
	
	ErrorInfo error;
	error.SetToV(code, CurrentFile(), CurrentLine(), e, vl);
	if( fParser ) fParser->Error(error);
	
	va_end(vl);
} /* error */

void ArrayParserState::Error(const char *e, ...) const
{
	va_list vl;
	va_start(vl, e);
	
	ErrorInfo error;
	error.SetToV(B_ERROR, CurrentFile(), CurrentLine(), e, vl);
	if( fParser ) fParser->Error(error);
	
	va_end(vl);
} /* error */

void ArrayParserState::Warn(const char *e, ...) const
{
	va_list vl;
	va_start(vl, e);
	
	ErrorInfo error;
	error.SetToV(0, CurrentFile(), CurrentLine(), e, vl);
	if( fParser ) fParser->Warn(error);
	
	va_end(vl);
} /* warn */

