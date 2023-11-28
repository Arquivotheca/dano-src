/*
	StyledTextToHtmlAdapter.cpp
*/
#include "StringBuffer.h"
#include "StyledTextToHtmlAdapter.h"
#include "www/util.h"

#define STTOHTMLADAPTORTEST 0

#if STTOHTMLADAPTORTEST
	#define STTOHTMLCHATTY 1
#else
	#define STTOHTMLCHATTY 0
#endif

#if STTOHTMLCHATTY
static const char *StateToText(int state);
#endif

enum {
	stateNoState = 0,
	stateScanningWhitespace,
	stateScanningWord,
	stateExaminingWord,
	stateReadNewBuffer,
	stateNewTextRun,
	stateCloseTextRun,
	statePushWordFragment,
	stateDumpHeader,
	stateDumpFooter,
	stateDumpWord,
	stateDumpStringStack,
	stateDoneDumpingStringStack,
	stateDumpingStack,
	stateDumpReplacementString,
	stateDumpHrefProlog,
	stateDumpHrefMiddle,
	stateDumpHrefEpilog,
	stateDumpingReplacementString,
	stateDumpingString,
	stateFinished,
};

// Taken from MerlinProtocol.cpp
// Should be consolidated. 
static const char * const kHeaderHtml = \
"<html><head></head><body text='#000000'>\n"
"<table border='0' cellpadding='15'><tr><td valign='top'><tt>";
static const char * const kFooterHtml = \
"\n</tt></td></tr></table>\n</body></html>";

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
	'\'',
	'\t'
};
	
static const char *kReplaceStrings[] = {
	"<br>",
	"",
	"&lt;",
	"&gt;",
	"&amp;",
	"&quot;",
	"&#x27;",
	"&nbsp;&nbsp;&nbsp;&nbsp;"
};

// must match len of strings above
static const unsigned int kReplaceStringsLen[] = {
	4,
	0,
	4,
	4,
	5,
	6,
	6,
	24
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

StyledTextToHtmlAdapter::StyledTextToHtmlAdapter(BDataIO *source, text_run_array *textRun, bool owning, bool includeHeaderFooter)
	:	fSource(source),
		fOwning(owning),
		fIncludeHeaderFooter(includeHeaderFooter),
		fInBufferSize(0),
		fInBufferPos(0),
		fTotalBytesPos(0),
		fTextRun(textRun),
		fTextRunIndex(0),
		fTextRunPos(0),
		fTextRunSize(0)
{
	// set up the initial states
	fStateStack.Push(stateScanningWhitespace);
	fStateStack.Push(stateDumpStringStack);
	fStateStack.Push(stateNewTextRun);
	fStateStack.Push(stateReadNewBuffer);
	if (fIncludeHeaderFooter)
		fStateStack.Push(stateDumpHeader);	
}

StyledTextToHtmlAdapter::~StyledTextToHtmlAdapter()
{
	// We are not responsible for the text run (SendMessageContainer is)
	if (fOwning)
		delete fSource;
}

ssize_t StyledTextToHtmlAdapter::Read(void *buffer, size_t size)
{
	ssize_t outPos = 0;
	char *buf = static_cast<char *>(buffer);
	
	if((ssize_t)size < 0) {
		// invalid size
		return B_ERROR;
	}

#if STTOHTMLCHATTY
	printf("StyledTextToHtmlAdapter::Read: %p entry size %d\n", this, size);
#endif
	for (;;) {
		ssize_t savedInBufferPos = fInBufferPos;
		if (outPos >= (ssize_t)size) {
			return outPos;
		}

#if STTOHTMLCHATTY
		printf("StyledTextToHtmlAdapter::Read: state '%s'\n", StateToText(fStateStack.ExamineHead()));
#endif
		switch (fStateStack.ExamineHead()) {
			case stateReadNewBuffer:
				fInBufferSize = fSource->Read(fInBuffer, sizeof(fInBuffer));
				if(fInBufferSize < 0)
					return fInBufferSize;
				if (fInBufferSize == 0) {
					fStateStack.Pop();
					// have it examine the word it has, if any and quit after that
					if(fStateStack.ExamineHead() == stateScanningWord) {
						// if we're scanning a word currently, check it for URL and dump it
						fStateStack.Push(stateFinished);
						if (fIncludeHeaderFooter)
							fStateStack.Push(stateDumpFooter);
						fStateStack.Push(stateDumpStringStack);
						fStateStack.Push(stateCloseTextRun);
						fStateStack.Push(stateExaminingWord);
						fStateStack.Push(statePushWordFragment);
					} else {
						fStateStack.Push(stateFinished);
						if (fIncludeHeaderFooter)
							fStateStack.Push(stateDumpFooter);
					}					
				} else {
					fStateStack.Pop();
					fInBufferPos = 0;
				}			
				break;
			case stateScanningWhitespace: {
				if(isspace(fInBuffer[fInBufferPos])) {
					bool stateTransition = false;
					for(unsigned int i=0; i<sizeof(kCharsToReplace)/sizeof(char); i++) {
						if(fInBuffer[fInBufferPos] == kCharsToReplace[i]) {
							// we need to replace this char
							fInBufferPos++;
							// have it dump the string replacement and return here
							fStateStack.Push(stateDumpReplacementString);
							fReplacementString = kReplaceStrings[i];
							stateTransition = true;
							break;
						}
					}
					if(!stateTransition) {
						buf[outPos++] = fInBuffer[fInBufferPos++];
					}
					// we had incremented the in buffer position, so increment the text run pos and total pos and see
					// if we need to move into another text run
					fTotalBytesPos++;
					fTextRunPos++;
					if(fTextRunPos >= fTextRunSize) {
						fStateStack.Push(stateDumpStringStack);
						fStateStack.Push(stateNewTextRun);
						fTextRunIndex++;
					}
				} else {
					// start scanning a word
					fStateStack.Pop();
					fStateStack.Push(stateScanningWord);
					fWordBufferPos = 0;
					fWordBufferLastPushedPos = 0;
				}
				break;
			}
			case stateScanningWord: {
				if(fInBuffer[fInBufferPos] == '\0') {
					fInBufferPos++;
					break;
				}
				bool toExamineState = false;
				if(!isspace(fInBuffer[fInBufferPos])) {
					fWordBuffer[fWordBufferPos++] = fInBuffer[fInBufferPos++];
					if(fWordBufferPos >= sizeof(fWordBuffer)) {
						// we just loaded a word that fills the buffer
						// go ahead and transition into scanning for URLS
						// and go to scanning a whitespace
						toExamineState = true;
					}
					// we had incremented the in buffer position, so increment the text run pos and total pos and see
					// if we need to move into another text run
					fTotalBytesPos++;
					fTextRunPos++;
					if(fTextRunPos >= fTextRunSize) {
						fStateStack.Push(stateNewTextRun);
						fStateStack.Push(stateCloseTextRun);
						if(!toExamineState)
							fStateStack.Push(statePushWordFragment);
						fTextRunIndex++;
					}
				} else {
					toExamineState = true;
				}
				if(toExamineState) {
					// go ahead and transition into scanning for URLS
					// and go to scanning a whitespace after that
					fStateStack.Pop();
					fStateStack.Push(stateScanningWhitespace);
					fStateStack.Push(stateExaminingWord);
					fStateStack.Push(statePushWordFragment);
				}
				break;
			}
			case statePushWordFragment:
				fStateStack.Pop();
				if(fWordBufferLastPushedPos < fWordBufferPos) {
					fStringFrag.u.str = &fWordBuffer[fWordBufferLastPushedPos];
					fStringFrag.flags = STRING_FRAG_FLAG_NONE;
					fStringFrag.strLen = fWordBufferPos - fWordBufferLastPushedPos;
					fStringStack.Push(fStringFrag);
					fWordBufferLastPushedPos = fWordBufferPos;
				}
				break;				
			case stateExaminingWord: {
				fStateStack.Pop(); // we will always transition to another state from here

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
					fStateStack.Push(stateDumpStringStack);
					break;
				}

				// check to make sure at least one character exists after this one
				if(scanPos >= fWordBufferPos) {
					fStateStack.Push(stateDumpStringStack);
					break;
				}

				// We're going to treat this one like a URL
				// Push the complex stack of things to do to deal with it
				fStateStack.Push(stateDumpHrefEpilog);
				fStateStack.Push(stateDumpStringStack);
				fStateStack.Push(stateDumpHrefMiddle);
				fStateStack.Push(stateDumpWord);
				fStateStack.Push(stateDumpHrefProlog);

				break;	
			}
			case stateNewTextRun: {
				fStateStack.Pop();

				if(fTextRun == NULL)
					break;
			
				text_run *run = &fTextRun->runs[fTextRunIndex];
				// XXX zip to the start of the new run
				fTextRunPos = 0;
				fTextRunSize = fTextRunIndex < (fTextRun->count - 1) ? fTextRun->runs[fTextRunIndex+1].offset - fTextRun->runs[fTextRunIndex].offset: 0x7fffffff;
				
				fCurrFontStringStack.Empty();

				StringFragment frag;
				frag.u.str = "<font face=\"Helvetica, Swiss, Arial\" size=\"";
				frag.flags = STRING_FRAG_FLAG_NOREPLACE;
				frag.strLen = strlen(frag.u.str);
				fStringStack.Push(frag);
				fCurrFontStringStack.Push(frag);

				long fontSize = (long) run->font.Size();
				if (fontSize < 10)
					frag.u.str = "-3";
				else if (fontSize >=10 && fontSize < 12)
					frag.u.str = "-2";
				else if (fontSize >= 12 && fontSize < 14)
					frag.u.str = "-1";
				else if (fontSize >= 14 && fontSize < 18)
					frag.u.str = "+0";
				else if (fontSize >= 18 && fontSize < 24)
					frag.u.str = "+1";
				else if (fontSize >= 24 && fontSize < 36)
					frag.u.str = "+2";
				else if (fontSize >= 36)
					frag.u.str = "+3";				
				
				frag.flags = STRING_FRAG_FLAG_NOREPLACE;
				frag.strLen = strlen(frag.u.str);
				fStringStack.Push(frag);
				fCurrFontStringStack.Push(frag);

				frag.u.str = "\" color=#";
				frag.flags = STRING_FRAG_FLAG_NOREPLACE;
				frag.strLen = strlen(frag.u.str);
				fStringStack.Push(frag);
				fCurrFontStringStack.Push(frag);
	
				frag.u.color = run->color;
				frag.flags = STRING_FRAG_FLAG_COLOR;
				frag.strLen = 0;
				fStringStack.Push(frag);
				fCurrFontStringStack.Push(frag);

				frag.u.str = ">";
				frag.flags = STRING_FRAG_FLAG_NOREPLACE;
				frag.strLen = strlen(frag.u.str);
				fStringStack.Push(frag);
				fCurrFontStringStack.Push(frag);
				
				if(run->font.Face() & B_BOLD_FACE) {
					frag.u.str = "<b>";
					frag.flags = STRING_FRAG_FLAG_NOREPLACE;
					frag.strLen = strlen(frag.u.str);
					fStringStack.Push(frag);
					fCurrFontStringStack.Push(frag);
				}
				break;	
			}
			case stateCloseTextRun: {
				fStateStack.Pop();
				if(fTextRun == NULL)
					break;
			
				if(fCurrFontStringStack.GetSize() > 0) {
					// push the </font> tag onto the string stack and
					// erase the curr font string stack
					StringFragment frag;
					
					frag = fCurrFontStringStack.ExamineHead();
					if(frag.flags != STRING_FRAG_FLAG_COLOR && strcmp(frag.u.str, "<b>") == 0) {
						frag.u.str = "</b>";
						frag.flags = STRING_FRAG_FLAG_NOREPLACE;
						frag.strLen = strlen(frag.u.str);
						fStringStack.Push(frag);
					}
					
					frag.u.str = "</font>";
					frag.flags = STRING_FRAG_FLAG_NOREPLACE;
					frag.strLen = strlen(frag.u.str);
					fStringStack.Push(frag);

					fCurrFontStringStack.Empty();
				}
				break;
			}
			case stateDumpHeader:
				fReplacementString = kHeaderHtml;
				fReplacementStringSize = strlen(fReplacementString);
				fReplacementStringDumpPos = 0;
				fStateStack.Pop();
				fStateStack.Push(stateDumpingReplacementString);
				break;
			case stateDumpFooter:
				fReplacementString = kFooterHtml;
				fReplacementStringSize = strlen(fReplacementString);
				fReplacementStringDumpPos = 0;
				fStateStack.Pop();
				fStateStack.Push(stateDumpingReplacementString);
				break;				
			case stateDumpWord:
				fStateStack.Pop();
				if(fWordBufferPos > 0) {
					fStringToDump = &fWordBuffer[0];
					fStringDumpPos = 0;
					fStringSize = fWordBufferPos;
					fStateStack.Push(stateDumpingString);
				}
				break;
			case stateDumpStringStack:
				fStateStack.Pop();
				if(fStringStack.GetSize() > 0) {
					fStackToDump = &fStringStack;
					fStringFragIndex = 0;
					fStringFragPos = 0;
					fStringFrag = fStringStack.GetNthItem(fStringFragIndex);
					fStateStack.Push(stateDoneDumpingStringStack);
					fStateStack.Push(stateDumpingStack);
				}
				break;
			case stateDoneDumpingStringStack:
				fStateStack.Pop();
				fStringStack.Empty();
				break;
			case stateDumpingStack: {
				if(fStringFragPos >= fStringFrag.strLen) {
					// move onto another string frag
					fStringFragIndex++;
					if(fStringFragIndex < fStackToDump->GetSize()) {
						fStringFrag = fStackToDump->GetNthItem(fStringFragIndex);						
						fStringFragPos = 0;
						if(fStringFrag.flags == STRING_FRAG_FLAG_COLOR) {
							// this is a color, so build a string for that and dump it
							rgb_color_to_html(fColorBuffer, fStringFrag.u.color);
							fStringFrag.u.str = fColorBuffer;
							fStringFrag.strLen = 6;
							fStringFrag.flags = STRING_FRAG_FLAG_NOREPLACE;
						}
					} else {
						// we hit the end of the stack
						fStateStack.Pop();
						break;
					}
				}

				if(fStringFrag.flags != STRING_FRAG_FLAG_NOREPLACE) {
					// see if we need to replace the char with something else
					bool stateTransition = false;
					for(unsigned int i=0; i<sizeof(kCharsToReplace)/sizeof(char); i++) {
						if(fStringFrag.u.str[fStringFragPos] == kCharsToReplace[i]) {
							// we need to replace this char
							fStringFragPos++;
							// have it dump the string replacement and return here
							fStateStack.Push(stateDumpReplacementString);
							fReplacementString = kReplaceStrings[i];
							stateTransition = true;
							break;
						}
					}
					if(stateTransition)
						break;
				}
				buf[outPos++] = fStringFrag.u.str[fStringFragPos++];
				break;
			}
			case stateDumpReplacementString:
				// fReplacementString should have been set up by someone else
				fReplacementStringSize = strlen(fReplacementString);
				fReplacementStringDumpPos = 0;
				fStateStack.Pop();
				fStateStack.Push(stateDumpingReplacementString);
				break;
			case stateDumpHrefProlog:
				fReplacementString = kHrefProlog;
				fReplacementStringSize = strlen(fReplacementString);
				fReplacementStringDumpPos = 0;
				fStateStack.Pop();
				fStateStack.Push(stateDumpingReplacementString);
				break;
			case stateDumpHrefMiddle:
				fReplacementString = kHrefMiddle;
				fReplacementStringSize = strlen(fReplacementString);
				fReplacementStringDumpPos = 0;
				fStateStack.Pop();
				fStateStack.Push(stateDumpingReplacementString);
				break;
			case stateDumpHrefEpilog:
				fReplacementString = kHrefEpilog;
				fReplacementStringSize = strlen(fReplacementString);
				fReplacementStringDumpPos = 0;
				fStateStack.Pop();
				fStateStack.Push(stateDumpingReplacementString);
				break;
			case stateDumpingReplacementString:
				if(fReplacementStringDumpPos >= fReplacementStringSize) {
					fStateStack.Pop();
					break;
				}
				buf[outPos++] = fReplacementString[fReplacementStringDumpPos++];
				break;
			case stateDumpingString: {
				if(fStringDumpPos >= fStringSize) {
					fStateStack.Pop();
					break;
				}

				bool stateTransition = false;
				for(unsigned int i=0; i<sizeof(kCharsToReplace)/sizeof(char); i++) {
					if(fStringToDump[fStringDumpPos] == kCharsToReplace[i]) {
						// we need to replace this char
						fStringDumpPos++;
						// have it dump the string replacement and return here
						fStateStack.Push(stateDumpReplacementString);
						fReplacementString = kReplaceStrings[i];
						stateTransition = true;
						break;
					}
				}
				if(stateTransition)
					break;

				buf[outPos++] = fStringToDump[fStringDumpPos++];
				break;
			}
			case stateFinished:
				return outPos;
			case stateNoState:
			default:
				// should never get here
				printf("StyledTextToHtmlAdapter::Read: got into invalid state!\n");
				return outPos;
				
		} // switch
		if(savedInBufferPos != fInBufferPos) {
			// see if we need to go to the readNewBuffer state
			if(fInBufferSize == 0 || fInBufferPos == fInBufferSize) {
				// yes we need a new buffer, so insert a readNewBuffer state
				// here by storing the state we were going to into fReadBufferNextState
				fStateStack.Push(stateReadNewBuffer);
			}
		}
	}

	return 0;
}

ssize_t StyledTextToHtmlAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}

#if STTOHTMLCHATTY
static const char *StateToText(int state)
{
	switch(state) {
		case stateNoState:
			return "stateNoState";
		case stateScanningWhitespace:
			return "stateScanningWhitespace";
		case stateScanningWord:
			return "stateScanningWord";
		case stateExaminingWord:
			return "stateExaminingWord";
		case stateReadNewBuffer:
			return "stateReadNewBuffer";
		case stateNewTextRun:
			return "stateNewTextRun";
		case stateCloseTextRun:
			return "stateCloseTextRun";
		case statePushWordFragment:
			return "statePushWordFragment";
		case stateDumpHeader:
			return "stateDumpHeader";
		case stateDumpFooter:
			return "stateDumpFooter";
		case stateDumpWord:
			return "stateDumpWord";
		case stateDumpStringStack:
			return "stateDumpStringStack";
		case stateDoneDumpingStringStack:
			return "stateDoneDumpingStringStack";
		case stateDumpingStack:
			return "stateDumpingStack";
		case stateDumpReplacementString:
			return "stateDumpReplacementString";
		case stateDumpHrefProlog:
			return "stateDumpHrefProlog";
		case stateDumpHrefMiddle:
			return "stateDumpHrefMiddle";
		case stateDumpHrefEpilog:
			return "stateDumpHrefEpilog";
		case stateDumpingReplacementString:
			return "stateDumpingReplacementString";
		case stateDumpingString:
			return "stateDumpingString";
		case stateFinished:
			return "stateFinished";
		default:
			return "unknownState";
	}
}
#endif

#if STTOHTMLADAPTORTEST
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

#if 0
char data[] =
"foobar"
;
#endif

#if 0
char data[] =
"This is a test\n"
"Second line\n"
"This line http://www.be.com/\n\n"
"this line has stuff in it like < or >\n"
"how about <>\r><'<\"&"
;
#endif

#if 1
char data[] =
"foo\n"
"https://www.be.com/"
;
#endif

int main(int argc, char *argv[])
{
	int fd;
	char buf[128];
	ssize_t bytes_read;
	
	BMallocIO store;
	store.WriteAt(0, data, sizeof(data));

	rgb_color color;
	text_run_array *array;
	array = BTextView::AllocRunArray(3);
	
	array->runs[0].offset = 0;
	array->runs[0].font = new BFont;
	color.red = 5;
	array->runs[0].color = color;
	
	array->runs[1].offset = 3;
	array->runs[1].font = new BFont;
	color.blue = 6;
	array->runs[1].color = color;

	array->runs[2].offset = 7;
	array->runs[2].font = new BFont;
	color.green = 9;
	array->runs[2].color = color;

	StyledTextToHtmlAdapter adapter(&store, array, false);

	fd = open("foo.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if(fd < 0)
		return 1;

	while((bytes_read = adapter.Read(buf, sizeof(buf))) > 0) {
		printf("read %d bytes from adaptor\n", bytes_read);
		write(fd, buf, bytes_read);
	}

	close(fd);

	return 0;
}
#endif
