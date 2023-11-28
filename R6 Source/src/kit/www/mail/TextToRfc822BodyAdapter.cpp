/*
	TextToRfc822BodyAdapter.cpp
*/
#include "TextToRfc822BodyAdapter.h"
#include <stdio.h>
#include <ctype.h>

#define TEXTTORFC822ADAPTORTEST 0

enum {
	stateReadNewBuffer,
	stateScanningWhitespace,
	stateScanningWhitespaceSeenCR,
	stateScanningWord,
	stateDumpingWord,
	stateEatingSpaces,
	stateCR,
	stateLF,
	stateFinalCRLF,
	stateFinalCRLF2,
	stateFinished,
};

TextToRfc822BodyAdapter::TextToRfc822BodyAdapter(BDataIO *source, int columnBreak, bool owning)
	:	fSource(source),
		fOwning(owning),
		fInBufferSize(0),
		fInBufferPos(0),
		fWordBufferPos(0),
		fWordDumpCount(0),
		fLineLen(0),
		fState(stateReadNewBuffer),
		fReadBufferNextState(stateScanningWhitespace),
		fColumnBreak(columnBreak)
{

}


TextToRfc822BodyAdapter::~TextToRfc822BodyAdapter()
{
	if (fOwning)
		delete fSource;
}

ssize_t TextToRfc822BodyAdapter::Read(void *buffer, size_t size)
{
	ssize_t outPos = 0;
	char *buf = static_cast<char *>(buffer);

	for(;;) {
		ssize_t savedInBufferPos = fInBufferPos;
		if(outPos >= size) {
			return outPos;
		}
		switch(fState) {
			case stateReadNewBuffer:
				fInBufferSize = fSource->Read(fInBuffer, sizeof(fInBuffer));
				if(fInBufferSize < 0)
					return fInBufferSize;
				if (fInBufferSize == 0) {
					// dump any word that may be cached up
					fState = stateDumpingWord;
					// after that, eat any whitespace
					fWordDumpNextState = stateEatingSpaces;
					// after that, Stuff two CRLF's
					fEatingSpacesNextState = stateFinalCRLF;
				} else {
					fInBufferPos = 0;
					fState = fReadBufferNextState;
				}
				break;	
			case stateScanningWhitespace:
				if(fInBuffer[fInBufferPos] == '\r') {
					fState = stateScanningWhitespaceSeenCR;
					fInBufferPos++; // eat the CR	
				} else if(fInBuffer[fInBufferPos] == '\n') {
					// we just saw a LF, so stick a CRLF in it's spot and move on
					fState = stateCR;
					fCRLFNextState = stateScanningWhitespace;
					fInBufferPos++; // eat the LF
				} else if(isspace(fInBuffer[fInBufferPos])) {
					if(fLineLen == fColumnBreak) {
						// we hit the wrap-around point, add a CRLF and have it return
						// to this state
						fState = stateCR;
						// have it eat spaces after the CRLF
						fCRLFNextState = stateEatingSpaces;
						// have it come back to here after that
						fEatingSpacesNextState = stateScanningWhitespace;
					} else {
						// copy the space directly to the output buffer
						buf[outPos++] = fInBuffer[fInBufferPos++];
						fLineLen++;
					}
				} else if(fInBuffer[fInBufferPos] == '\0') {
					// we hit a null, skip it
					fInBufferPos++;
				} else {
					// we hit an alphanumeric or something else character,
					// treat it like the start of a word
					fWordBufferPos = 0;
					fState = stateScanningWord;
				}
				break;
			case stateScanningWhitespaceSeenCR:
				// we had previously seen a CR and are waiting to see whats next
				// always output a CRLF, but eat a LF if that was next, otherwise
				// we'll be turning a single CR to a CRLF
				if(fInBuffer[fInBufferPos] == '\n') { 
					// we just saw a CRLF, so output one and move on
					fInBufferPos++; // eat the LF
				}	
				fState = stateCR;
				fCRLFNextState = stateScanningWhitespace;
				break;
			case stateScanningWord:
				if(!isspace(fInBuffer[fInBufferPos]) && fInBuffer[fInBufferPos] != '\0') {
					// save the character of this word
					fWordBuffer[fWordBufferPos++] = fInBuffer[fInBufferPos++];
					// see if this word itself is longer than an entire line
					if(fWordBufferPos >= fColumnBreak) {
						// this is one long word!
						// go ahead and dump a CRLF then dump this part of the word 
						// and come back to this state
						fState = stateCR;
						fCRLFNextState = stateDumpingWord;
						fWordDumpNextState = stateScanningWord;
					}
				} else {
					// we're not looking at an alphanumeric now
					// dump the word we've been scanning
					if(fWordBufferPos + fLineLen > fColumnBreak) {
						// if we dumped the word, we'd go past the wrap column,
						// so insert a CRLF here,
						fState = stateCR;
						 // then dump the word
						fCRLFNextState = stateDumpingWord;
					} else {
						// otherwise, dump the word right now
						fState = stateDumpingWord;
					}
					// after that, scan whitespace for other stuff
					fWordDumpNextState = stateScanningWhitespace;
				}
				break;
			// the next state dumps whatever is in the word buffer and move
			// to the state stored in fWordDumpNextState
			case stateDumpingWord:
				if(fWordDumpCount < fWordBufferPos) {
					buf[outPos++] = fWordBuffer[fWordDumpCount++];
					fLineLen++;
				} else {
					// we're done dumping this word
					fState = fWordDumpNextState;
					fWordDumpCount = 0;
					fWordBufferPos = 0;
				}
				break;
			case stateEatingSpaces:
				if(isspace(fInBuffer[fInBufferPos])) {
					fInBufferPos++; // eat this space
				} else {
					fState = fEatingSpacesNextState;
				}
				break;
			// the next two states push a CRLF pair then move to the state
			// stored in fCRLFNextState
			case stateCR:
				fLineLen = 0;
				buf[outPos++] = '\r';
				fState = stateLF;
				break;
			case stateLF:
				buf[outPos++] = '\n';
				fState = fCRLFNextState;
				break;
			// the next two states push a final pair of CRLFs
			// then move to the finished state
			case stateFinalCRLF:
				fState = stateCR;
				fCRLFNextState = stateFinalCRLF2;
				break;
			case stateFinalCRLF2:
				fState = stateCR;
				fCRLFNextState = stateFinished;
			// final state
			case stateFinished:
				return outPos;
				break;
			default:
				break;
		} // switch(fState)

		// see if we need to go to the readNewBuffer state
		if(savedInBufferPos != fInBufferPos && (fInBufferSize == 0 || fInBufferPos == fInBufferSize)) {
			// yes we need a new buffer, so insert a readNewBuffer state
			// here by storing the state we were going to into fReadBufferNextState
			fReadBufferNextState = fState;
			fState = stateReadNewBuffer;
		}
	}

	return outPos;
}

ssize_t TextToRfc822BodyAdapter::Write(const void * /* buffer */, size_t /* size */)
{
	return B_ERROR;
}

#if TEXTTORFC822ADAPTORTEST
#include <unistd.h>
#include <fcntl.h>

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
int main(int argc, char *argv[])
{
	int fd;
	char buf[3];
	ssize_t bytes_read;
	
	BMallocIO store;
	store.WriteAt(0, data, sizeof(data));
	
	TextToRfc822BodyAdapter adapter(&store, false);

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
