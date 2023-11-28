#include <stdio.h>
#include <List.h>
#include <ctype.h>
#include <string.h>
#include "TextToHtmlAdapter.h"

#define TEXTTOHTMLADAPTORTEST 0

#if TEXTTOHTMLADAPTORTEST
	#define TEXTTOHTMLADAPTORCHATTY 1
#else
	#define TEXTTOHTMLADAPTORCHATTY 0
#endif

#if TEXTTOHTMLADAPTORCHATTY
static const char *StateToText(int state);
#endif

enum {
	stateNoState,
	stateScanningWhitespace,
	stateScanningWord,
	stateDumpingReplacementString,
	stateExaminingWord,
	stateDumpWord,
	stateDumpingWord,
	stateDumpHrefProlog,
	stateDumpingHrefProlog,
	stateDumpHrefMiddle,
	stateDumpingHrefMiddle,
	stateDumpHrefEpilog,
	stateDumpingHrefEpilog,
	stateReadNewBuffer,
	stateFinished,
};

static const char *kHrefProlog = "<a href=\"";
static const char *kHrefMiddle = "\">";
static const char *kHrefEpilog = "</a>";

// any addition to the next array must be made to two after that
static const char kCharsToReplace[] = {
	'\n',
	'\r',
	'<',
	'>',
	'&',
	'"',
	'\''
};
	
static const char *kReplaceStrings[] = {
	"<br>",
	"",
	"&lt;",
	"&gt;",
	"&amp;",
	"&quot;",
	"&#x27;"
};

// must match len of strings above
static const unsigned int kReplaceStringsLen[] = {
	4,
	0,
	4,
	4,
	5,
	6,
	6
};

// any addition here must be made to the array after this one
static const char *kURLProtocol[] = {
	"https://",
	"http://",
	"ftp://",
	"mailto:",
	NULL
};

// must match len of strings above
static const unsigned int kURLProtocolLen[] = {
	5,
	4,
	3,
	6,
	0
};

TextToHtmlAdapter::TextToHtmlAdapter(BDataIO *inText, bool owning)
	:	fSource(inText),
		fInBufferSize(0),
		fInBufferPos(0),
		fOwning(owning),
		fWordBufferPos(0),
		fWordDumpPos(0),
		fStringDumpPos(0),
		fStringToReplace(0)
{
	// set up the initial states
	fStateStackPos = -1;
	PushState(stateScanningWhitespace);
	PushState(stateReadNewBuffer);
}

TextToHtmlAdapter::~TextToHtmlAdapter()
{
	if (fOwning)
		delete fSource;
}

ssize_t TextToHtmlAdapter::Read(void *buffer, size_t size)
{
	ssize_t outPos = 0;
	char *buf = static_cast<char *>(buffer);
	
	if((ssize_t)size < 0) {
		// invalid size
		return B_ERROR;
	}
	
#if TEXTTOHTMLADAPTORCHATTY
	printf("TextToHtmlAdapter::Read: %p entry size %ld\n", this, size);
#endif	
	for (;;) {
		ssize_t savedInBufferPos = fInBufferPos;
		if (outPos >= (ssize_t)size) {
			return outPos;
		}
#if TEXTTOHTMLADAPTORCHATTY
		printf("TextToHtmlAdapter::Read: state '%s'\n", StateToText(ExamineStateHead()));
		printf("\tfInBufferSize %d fInBufferPos %d fWordBufferPos %d fWordDumpPos %d fStringDumpPos %d fStringToReplace %d\n",
			fInBufferSize, fInBufferPos, fWordBufferPos, fWordDumpPos, fStringDumpPos, fStringToReplace);
		DumpStateStack();
#endif
		switch (ExamineStateHead()) {
			case stateReadNewBuffer:
				fInBufferSize = fSource->Read(fInBuffer, sizeof(fInBuffer));
				if(fInBufferSize < 0)
					return fInBufferSize;
				if (fInBufferSize == 0) {
					PopState();
					// have it examine the word it has, if any and quit after that
					if(ExamineStateHead() == stateScanningWord) {
						// if we're scanning a word currently, check it for URL and dump it
						PushState(stateFinished);
						PushState(stateExaminingWord);
					} else {
						PushState(stateFinished);
					}						
				} else {
					PopState();
					fInBufferPos = 0;
				}			
				break;
			case stateScanningWhitespace: {
				bool stateTransition = false;
				for(unsigned int i=0; i<sizeof(kCharsToReplace)/sizeof(char); i++) {
					if(fInBuffer[fInBufferPos] == kCharsToReplace[i]) {
						// we need to replace this char
						fInBufferPos++;
						// have it dump the string replacement and return here
						PushState(stateDumpingReplacementString);
						fStringDumpPos = 0;
						fStringToReplace = i;
						stateTransition = true;
						break;
					}
				}
				if(stateTransition)
					break;
				
				if(isspace(fInBuffer[fInBufferPos])) {
					buf[outPos++] = fInBuffer[fInBufferPos++];
				} else {
					// start scanning a word
					PopState();
					PushState(stateScanningWord);
					fWordBufferPos = 0;
				}
				break;
			}
			case stateScanningWord: {
				bool toExamineState = false;
				if(!isspace(fInBuffer[fInBufferPos])) {
					fWordBuffer[fWordBufferPos++] = fInBuffer[fInBufferPos++];
					if(fWordBufferPos >= sizeof(fWordBuffer)) {
						// we just loaded a word that fills the buffer
						// go ahead and transition into scanning for URLS
						// and go to scanning a whitespace
						toExamineState = true;
					}
				} else {
					toExamineState = true;
					
				}
				if(toExamineState) {
					// go ahead and transition into scanning for URLS
					// and go to scanning a whitespace after that
					PopState();
					PushState(stateScanningWhitespace);
					PushState(stateExaminingWord);
				}
				break;
			}
			case stateDumpingReplacementString:	
				if(fStringDumpPos >= kReplaceStringsLen[fStringToReplace]) {
					PopState();
					break;
				}
				buf[outPos++] = kReplaceStrings[fStringToReplace][fStringDumpPos++];
				break;
			case stateExaminingWord: {
				PopState(); // we will always transition to another state from here
				// look for a match of the protocol word
				unsigned int scanPos = 0;
				for(unsigned int i = 0; kURLProtocol[i] != NULL; i++) {
					if(fWordBufferPos > kURLProtocolLen[i]) {
						if(strncmp(fWordBuffer, kURLProtocol[i], kURLProtocolLen[i]) == 0) {
							// we have a match
							scanPos = kURLProtocolLen[i];
							break;
						}
					}
				}
				if(scanPos == 0) {
					PushState(stateDumpWord);
					break;
				}
								
				// check to make sure at least one character exists after this one
				if(scanPos >= fWordBufferPos) {
					PushState(stateDumpWord);
					break;
				}
				
				// There might be crap at the end of the URL that we don't
				// want.  Put it back in the input for later use.
				
				while (fWordBufferPos >= 1 &&
				 BadEndChar(fWordBuffer[fWordBufferPos - 1])) {
				     fWordBufferPos--;
				     
				     if (fInBufferPos >= 1) {
					     fInBufferPos--;
					     fInBuffer[fInBufferPos] = fWordBuffer[fWordBufferPos];
				     }
				     
				     fWordBuffer[fWordBufferPos] = 0;
				   }
				
				
				// We're going to treat this one like a URL
				// Push the complex stack of things to do to deal with it
				
				PushState(stateDumpHrefEpilog);
				PushState(stateDumpWord);
				PushState(stateDumpHrefMiddle);
				PushState(stateDumpWord);
				PushState(stateDumpHrefProlog);

				break;	
			}
			case stateDumpWord:
				PopState();
				if(fWordBufferPos > 0) {
					PushState(stateDumpingWord);
					fWordDumpPos = 0;
				}
				break;
			case stateDumpingWord: {
				if(fWordDumpPos >= fWordBufferPos) {
					PopState();
					break;
				}
				
				bool stateTransition = false;
				for(unsigned int i=0; i<sizeof(kCharsToReplace)/sizeof(char); i++) {
					if(fWordBuffer[fWordDumpPos] == kCharsToReplace[i]) {
						// we need to replace this char
						fWordDumpPos++;
						// have it dump the string replacement and return here
						PushState(stateDumpingReplacementString);
						fStringDumpPos = 0;
						fStringToReplace = i;
						stateTransition = true;
						break;
					}
				}
				if(stateTransition)
					break;

				buf[outPos++] = fWordBuffer[fWordDumpPos++];
				break;
			}
			case stateDumpHrefProlog:
				PopState();
				PushState(stateDumpingHrefProlog);
				fStringDumpPos = 0;
				break;
			case stateDumpingHrefProlog:
				if(fStringDumpPos >= strlen(kHrefProlog)) {
					PopState();
					break;
				}
				buf[outPos++] = kHrefProlog[fStringDumpPos++];
				break;
			case stateDumpHrefMiddle:
				PopState();
				PushState(stateDumpingHrefMiddle);
				fStringDumpPos = 0;
				break;
			case stateDumpingHrefMiddle:
				if(fStringDumpPos >= strlen(kHrefMiddle)) {
					PopState();
					break;
				}
				buf[outPos++] = kHrefMiddle[fStringDumpPos++];
				break;
			case stateDumpHrefEpilog:
				PopState();
				PushState(stateDumpingHrefEpilog);
				fStringDumpPos = 0;
				break;
			case stateDumpingHrefEpilog:
				if(fStringDumpPos >= strlen(kHrefEpilog)) {
					PopState();
					break;
				}
				buf[outPos++] = kHrefEpilog[fStringDumpPos++];
				break;
			case stateFinished:
#if TEXTTOHTMLADAPTORCHATTY
				printf("returning %ld\n", outPos);
				for(int j = 0; j < outPos; j++) {
					printf("0x%x %c\n", buf[j], buf[j]);
				}
#endif
				return outPos;
			case stateNoState:
			default:
				// should never get here
				printf("TextToHtmlAdapter::Read: got into invalid state!\n");
				return outPos;
		} // switch
		// see if we need to go to the readNewBuffer state
		if(savedInBufferPos != fInBufferPos && (fInBufferSize == 0 || fInBufferPos == fInBufferSize)) {
			// yes we need a new buffer, so insert a readNewBuffer state
			// here by storing the state we were going to into fReadBufferNextState
			PushState(stateReadNewBuffer);
		}
	}		
						
	return outPos;
}

ssize_t TextToHtmlAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}

inline int TextToHtmlAdapter::GetStateStackSize()
{
	return fStateStackPos;
}

inline int TextToHtmlAdapter::ExamineStateHead()
{
	if (fStateStackPos < 0)
		return stateNoState;
		
	return fStateStack[fStateStackPos];
}

inline int TextToHtmlAdapter::PopState()
{
	if (fStateStackPos < 0)
		return stateNoState;
		
	return fStateStack[fStateStackPos--];
}

inline int TextToHtmlAdapter::PushState(int state)
{
	if (fStateStackPos >= (((int)sizeof(fStateStack) / (int)sizeof(fStateStack[0])) - 1))
		return B_ERROR;
	
	fStateStack[++fStateStackPos] = state;

	return B_OK;
}

void TextToHtmlAdapter::DumpStateStack()
{
#if TEXTTOHTMLADAPTORCHATTY
	printf("State Stack: num items %d : ", fStateStackPos + 1);
	for(int i = 0; i<=fStateStackPos; i++) {
		printf("'%s' ", StateToText(fStateStack[i]));
	}
	printf("\n");
#endif
}

bool TextToHtmlAdapter::BadEndChar(char ch)
{
	// These are taken from the behavior of Hotmail.
	switch (ch) {
		case '.':
		case ',':
		case '!':
		case '(':
		case ')':
		case '\'':
		case '\"':
		case ':':
		case ';':
		case '?':
			return true;

		default:
			return false;
    }
}

#if TEXTTOHTMLADAPTORCHATTY
static const char *StateToText(int state)
{
	switch(state) {
		case stateNoState:
			return "stateNoState";
		case stateScanningWhitespace:
			return "stateScanningWhitespace";
		case stateScanningWord:
			return "stateScanningWord";
		case stateDumpingReplacementString:
			return "stateDumpingReplacementString";
		case stateExaminingWord:
			return "stateExaminingWord";
		case stateDumpWord:
			return "stateDumpWord";
		case stateDumpingWord:
			return "stateDumpingWord";
		case stateDumpHrefProlog:
			return "stateDumpHrefProlog";
		case stateDumpingHrefProlog:
			return "stateDumpingHrefProlog";
		case stateDumpHrefMiddle:
			return "stateDumpHrefMiddle";
		case stateDumpingHrefMiddle:
			return "stateDumpingHrefMiddle";
		case stateDumpHrefEpilog:
			return "stateDumpHrefEpilog";
		case stateDumpingHrefEpilog:
			return "stateDumpingHrefEpilog";
		case stateReadNewBuffer:
			return "stateReadNewBuffer";
		case stateFinished:
			return "stateFinished";
		default:
			return "unknownState";
	}
}
#endif

#if TEXTTOHTMLADAPTORTEST
#include <unistd.h>
#include <fcntl.h>

#if 0
char data[] =
//           1         2         3         4         5         6         7         8         9
// 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
  "This is a test of the adaptor.\n"
  "This line will wrap around and should be cut off at a particular point right about here.\n"
  "now I will put a very long word asdflaksdjflaskdfjalskdfjalsdkfjalsdkfjalskdfjalsdkfjalsdkfjalskfdjalsdkfjalskdfjsadf and some text after it.\n"
  "now I will have a bunch of spaces in a row:\n"
  "                                                                                           \n"
  "now I will have a few lines with spaces at the beginning:\n"
  "     one\n"
  "        two\n"
;
#endif

#if 1
char data[] =
	"http://www.be.com/index.html.\n"
	"http://www.be.com/index.html,\n"
	"http://www.be.com/index.html!\n"
	"http://www.be.com/index.html@\n"
	"http://www.be.com/index.html#\n"
	"http://www.be.com/index.html$\n"
	"http://www.be.com/index.html%\n"
	"http://www.be.com/index.html^\n"
	"http://www.be.com/index.html&\n"
	"http://www.be.com/index.html*\n"
	"http://www.be.com/index.html(\n"
	"http://www.be.com/index.html)\n"
	"http://www.be.com/index.html+\n"
	"http://www.be.com/index.html=\n"
	"http://www.be.com/index.html_\n"
	"http://www.be.com/index.html-\n"
	"http://www.be.com/index.html'\n"
	"http://www.be.com/index.html\"\n"
	"http://www.be.com/index.html:\n"
	"http://www.be.com/index.html;\n"
	"http://www.be.com/index.html?\n"
;
#endif

#if 0
char data[] =
"This is a test\n"
"Second line\n"
"This line http://www.be.com/.\n\n"
"this line has stuff in it like < or >\n"
"how about <>\r><'<\"&"
;
#endif

#if 0
char data[] =
"https://www.be.com/fooon/<that>/sdf&asdf"
;
#endif

int main(int argc, char *argv[])
{
	int fd;
	char buf[3];
	ssize_t bytes_read;
	
	BMallocIO store;
	store.WriteAt(0, data, sizeof(data));
	
	TextToHtmlAdapter adapter(&store, false);

	fd = open("foo.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if(fd < 0)
		return 1;

	while((bytes_read = adapter.Read(buf, sizeof(buf))) > 0) {
		printf("read %ld bytes from adaptor\n", bytes_read);
		write(fd, buf, bytes_read);
	}

	close(fd);

	return 0;
}
#endif
