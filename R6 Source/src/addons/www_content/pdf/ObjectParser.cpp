#include "ObjectParser.h"
#include "Object2.h"
#include "pdf_doc.h"
#include "ObjectSink.h"
#include "lex_maps.h"

#define LEFT_PAREN '('
#define RIGHT_PAREN ')'
#define LEFT_ANGLE '<'
#define RIGHT_ANGLE '>'
#define LEFT_SQUARE '['
#define RIGHT_SQUARE ']'
#define LEFT_CURLY '{'
#define RIGHT_CURLY '}'
#define BACKSLASH '\\'

uint32 whitemap[8] = {
	0x00003600, 0x00000001, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static uint32 eolmap[8] = {
	0x00002400, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static uint32 nummap[8] = {
	0x00000000, 0x03ff6800, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static uint32 wordmap[8] = {
	~0x00003601, ~0x50008321, ~0x28000000, ~0x28000000,
	~0x00000000, ~0x00000000, ~0x00000000, ~0x00000000
};


ObjectParser::ObjectParser(ObjectSink *sink)
	: Pusher(sink), fObjectSink(sink), fType(LEX_UNKNOWN), fDoingEscape(0), fDoingRawWrites(false),
		fDone(false)
{
}


ObjectParser::~ObjectParser()
{
}

void
ObjectParser::SniffTokenType(uint8 aByte)
{
	// start with a clean slate
	fString.Clear();
	fTokenFinished = false;
	// determine type of token
	switch (aByte)
	{
		case LEFT_PAREN:
			fType = LEX_PAREN_STRING;
			fParenDepth = 0;
			break;
		case LEFT_ANGLE:
			fType = LEX_HEX_STRING;
			break;
		case RIGHT_ANGLE:
			// this had *better* be and end-of-dictionary token...
			fType = LEX_KEYWORD;
			fString.Append((char)aByte);
			break;
		case LEFT_CURLY:	// these two are reserved, but unused in PDF
		case RIGHT_CURLY:	// ...
		case LEFT_SQUARE:	// these two begin and end arrays
		case RIGHT_SQUARE:	// ...
			fType = LEX_KEYWORD;
			fString.Append((char)aByte);
			fTokenFinished = true;
			break;

		case '/':
			// parse a name
			fType = LEX_NAME;
			break;

		case '%':
			// parse comment
			fType = LEX_COMMENT;
			break;

		case ' ':
		case '\t':
			// ignore white space
		case '\f':
			// formfeed is an EOL marker
		case '\n':
			// bare newline is an EOL marker
		case '\r':
			// carrige return may be by itself or followed by a newline
			// ignore it
			break;

		case '.':
		case '+':
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fType = LEX_NUMBER;
			fNumber = 0;
			fHasDecimal = 0;
			fNegate = false;
			break;

		default:
			fType = LEX_KEYWORD;
			// push back this character
			fString.Append((char)aByte);
			break;
	}
}

bool
ObjectParser::DoHex(uint8 aByte)
{
	bool reuseByte = false;

	switch (aByte)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fHexByte *= 16;
			fHexByte += aByte - '0';
			fDoingEscape++;
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			fHexByte *= 16;
			fHexByte += aByte - ('a' - 10);
			fDoingEscape++;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			fHexByte *= 16;
			fHexByte += aByte - ('A' - 10);
			fDoingEscape++;
			break;
		case '>':
			// special case for one char hex byte at end of string
			// also gets picked up for the invalid /name#A>> case
			if (fDoingEscape == 2)
			{
				fHexByte *= 16;
				fDoingEscape++;
			}
			else
				fDoingEscape = 0;
			reuseByte = true;
			break;
		default:
			// hmmm, non hex char.  let's try a graceful exit
			// run this byte through the grinder again
			fDoingEscape = 0;
			reuseByte = true;
			break;
	}
	// done processing byte?
	if (fDoingEscape == 3)
	{
		fString.Append((char)fHexByte);
		fDoingEscape = 0;
		fHexByte = 0;
	}
	return reuseByte;
}

bool
ObjectParser::ParenStringEscape(uint8 aByte)
{
	bool reuseByte = false;

	// is this an escaped \r only sequence
	if ((fDoingEscape == 5) && (aByte != '\n'))
	{
		// stop the escape
		fDoingEscape = 0;
		// run this char through the grinder again
		return true;
	}

	switch (aByte)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fDoingEscape++;
			fHexByte *= 8;
			fHexByte += aByte - '0';
			break;
		case 'n':	// newline escape
		case 'r':	// carrige return escape
		case 't':	// tab
		case 'b':	// backspace
		case 'f':	// formfeed
			if (fDoingEscape > 1)
			{
				reuseByte = true;
			}
			else
			{
				static const char *keys = "nrtbf";
				static const char *vals = "\n\r\t\b\f";
				char *s = strchr(keys, aByte);
				fHexByte = vals[s - keys];
			}
			fDoingEscape = 4;
			break;
		case LEFT_PAREN:
		case RIGHT_PAREN:
		case BACKSLASH:
			fDoingEscape = 4;
			fHexByte = aByte;
			break;
		case '\r':
			// a plain escaped \r
			if (fDoingEscape == 1) fDoingEscape = 5;
			else
			{
				// a \r after an incomplete octal escape
				fDoingEscape = 4;
				reuseByte = true;
			}
			break;
		case '\n':
			// a plain escaped \n
			if (fDoingEscape == 1) fDoingEscape = 0;
			else
			{
				// a \n after an incomplete octal escape
				fDoingEscape = 4;
				reuseByte = true;
			}
			break;
		default:
			if (fDoingEscape > 1)
			{
				fDoingEscape = 4;
				reuseByte = true;
			}
			break;
	}
	// did escape plus three octal chars?
	if (fDoingEscape == 4)
	{
		fString.Append((char)fHexByte);
		fDoingEscape = 0;
		fHexByte = 0;
	}
	return reuseByte;
}

ssize_t 
ObjectParser::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	if (fDone) return Pusher::SINK_FULL;
	ssize_t origLength = length;
	status_t result = B_OK;

	while (length && !fDone)
	{
		if (fDoingRawWrites)
		{
			//printf("RawWrite(%p, %ld) ->%.*s<-\n", buffer, length, length, buffer);
			result = fObjectSink->Write(buffer, length, finish);
			if (result >= 0)
			{
				buffer += result;
				length -= result;
			}
			else
			{
				if (result == ObjectSink::WANT_OBJECTS) fDoingRawWrites = false;
				else break;
			}
		}
		else
		{
			//printf("WriteObjects(%p, %ld) ->%.*s<-\n", buffer, length, length, buffer);
			result = WriteObjects(buffer, length, finish);
			if (result >= 0)
			{
				buffer += result;
				length -= result;
			}
			else
			{
				if (result == Pusher::SINK_FULL)
				{
					result = Pusher::OK;
					fDone = true;
				}
				break;
			}
		}
	}
	if (finish)
	{
		fDoingRawWrites = false;
		fType = LEX_UNKNOWN;
	}
	return (result >= 0) ? origLength - length : result;
}

ssize_t 
ObjectParser::WriteObjects(const uint8 *buffer, ssize_t length, bool finish)
{
	//
	uint8 aByte;
	bool reuseByte = false;
	ssize_t origLength = length;
	status_t result = OK;

start:
	while (length)
	{
		aByte = *buffer;
restart:
		switch (fType)
		{
			case LEX_UNKNOWN:
			{
				SniffTokenType(aByte);
				// short-circut LEX_NUMBER because we don't build a number string
				if (fType == LEX_NUMBER) goto restart;
			} break;
			case LEX_COMMENT:
			{
				if (InMap(eolmap, aByte))
				{
					// ignore the comment
					fType = LEX_UNKNOWN;
				}
			} break;
			case LEX_NAME:
			{
				if (fDoingEscape) reuseByte = DoHex(aByte);
				else
				{
					// done if invalid character read
					if (InMap(wordmap, aByte))
					{
						// the hex digit escape sequence?
						if (aByte == '#') fDoingEscape++;
						// I guess not
						else fString.Append((char)aByte);
					}
					else
					{
						fTokenFinished = true;
						reuseByte = true;
					}
				}
			} break;
			case LEX_PAREN_STRING:
			{
				// do a paren string
				// an escape character?
				if (fDoingEscape)
					reuseByte = ParenStringEscape(aByte);
				else if (aByte == '\\')
					fDoingEscape = 1;
				else if (aByte == LEFT_PAREN)
					fParenDepth++;
				else if (aByte == RIGHT_PAREN)
				{
					if (fParenDepth == 0)
						fTokenFinished = true;
					else
					{
						fParenDepth--;
						fString.Append((char)aByte);
					}
				}
				else
					fString.Append((char)aByte);
			} break;
			case LEX_HEX_STRING:
			{
				// do a hex string, unless we have started a dictionary
				if (aByte == LEFT_ANGLE)
				{
					fType = LEX_KEYWORD;
					fTokenFinished = true;
					fString.Append((char)aByte);
					fString.Append((char)aByte);
				}
				else if (!InMap(whitemap, aByte))
				{
					// make the code think we're doing hex digits
					if (fDoingEscape == 0) fDoingEscape++;
					if ((reuseByte = DoHex(aByte)) && (aByte == RIGHT_ANGLE))
					{
						fTokenFinished = true;
						reuseByte = false;
					}
				}
			} break;
			case LEX_KEYWORD:
			{
				// done if invalid character read
				if (InMap(wordmap, aByte))
				{
					fString.Append((char)aByte);
				}
				else
				{
					if (fString.Length() && (fString.String()[fString.Length()-1] == '>'))
					{
						fString.Append((char)aByte);
					}
					else
					{
						reuseByte = true;
					}
					fTokenFinished = true;
				}
			} break;
			case LEX_NUMBER:
			{
				if (InMap(nummap, aByte))
				{
					if (aByte == '.')
					{
						if (fHasDecimal > 0)
						{
							// bogus?
							fTokenFinished = true;
							reuseByte = true;
						}
						else
						{
							fHasDecimal = 1;
						}
					}
					else if (aByte == '-')
					{
						if (fNegate)
						{
							// bogus?
							fTokenFinished = true;
							reuseByte = true;
						}
						else
						{
							fNegate = true;
						}
					}
					else if (aByte != '+')
					{
						fNumber *= 10;
						fNumber += aByte - '0';
						fHasDecimal *= 10;
					}
				}
				else
				{
					fTokenFinished = true;
					reuseByte = true;
				}
			} break;
			default:
				assert(0 == 1);
				break;
		}
		if (fTokenFinished)
		{
			// make the object
			const char *string = fString.String();
			PDFObject *o = 0;
			switch (fType)
			{
				case LEX_NAME:
					o = PDFObject::makeName(string);
					break;
				case LEX_PAREN_STRING:
				case LEX_HEX_STRING:
					o = PDFObject::makeString(fString.Length(), string);
					break;
				case LEX_KEYWORD:
					if (strcmp(string, "true") == 0)
						o = PDFObject::makeBoolean(true);
					else if (strcmp(string, "false") == 0)
						o = PDFObject::makeBoolean(false);
					else if (strcmp(string, "null") == 0)
						o = PDFObject::makeNULL();
					else
						o = PDFObject::makeKeyword(string);
					break;
				case LEX_NUMBER:
				{
					double aNumber = (double)fNumber;
					if (fHasDecimal) aNumber /= (double)fHasDecimal;
					if (fNegate) aNumber *= -1.0;
					o = PDFObject::makeNumber(aNumber);
				} break;
				default:
					assert(0 == 1);
					break;
			}
			// reset the parser
			fType = LEX_UNKNOWN;
			fTokenFinished = false;
			// push the object to our sink (they take ownership of the object)
			result = fObjectSink->Write(o);
			if (result == ObjectSink::WANT_RAW_DATA)
			{
				fDoingRawWrites = true;
				result = ObjectSink::OK;
				break;
			}
			else if (result != ObjectSink::OK)
			{
				if (result == Pusher::SINK_FULL) result = Pusher::OK;
				fDone = true;
				break;
			}
		}
		if (reuseByte)
		{
			reuseByte = false;
			continue;
		}
		// process next char
		length--;
		buffer++;
	}
	// end of input but incomplete token
	if (finish && (length == 0) && (fType != LEX_UNKNOWN))
	{
		// ignore strings (really borked!)
		if (!((fType == LEX_PAREN_STRING) || (fType == LEX_HEX_STRING)))
		{
			// but finish off names, keywords and numbers
			buffer = (const uint8 *)" "; length = 1;
			goto start;
		}
	}
	return (result != ObjectSink::OK) ? result : origLength - length;
}

