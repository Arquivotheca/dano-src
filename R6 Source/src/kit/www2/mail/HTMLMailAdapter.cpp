#include <ctype.h>
#include <stdio.h>
#include <URL.h>
#include "HTMLMailAdapter.h"

using namespace Wagner;

const char *kSourceTags[] = {
	"background",
	"bgsound",
	"src"
};

HTMLMailAdapter::HTMLMailAdapter(MimeMessage *message, BDataIO *source, bool owning)
	:	fMessage(message),
		fSource(source),
		fOwning(owning),
		fState(kScanText),
		fQuoted(false),
		fInBufferPos(0),
		fReadBufferSize(0)
{
}

HTMLMailAdapter::~HTMLMailAdapter()
{
	if (fOwning)
		delete fSource;
}

ssize_t HTMLMailAdapter::Read(void *buffer, size_t size)
{
	char *buf = reinterpret_cast<char*>(buffer);
	ssize_t totalRead = 0;
	const char *name;
	while (totalRead < size) {
		char c;
		if (fState != kReplace) {
			if (fInBufferPos == fReadBufferSize) {
				fInBufferPos = 0;
				fReadBufferSize = fSource->Read(fInBuffer, kMaxBufferSize);
				if (fReadBufferSize < 1)
					break;
			}
			c = fInBuffer[fInBufferPos++];
		}
			
		switch (fState) {
			case kScanText:
				if (c == '<')
					fState = kScanTag;

				buf[totalRead++] = c;
				break;
				
			case kScanTag:
				if (c == '>')
					fState = kScanText;
				else if (c == '"')
					fState = kScanQuotedString;
				else if (!isspace(c)) {
					fCurrentName.Clear();
					fCurrentName << c;
					fState = kScanName;
				}
				
				buf[totalRead++] = c;
				break;
				
			case kScanQuotedString:
				if (c == '"')
					fState = kScanTag;

				buf[totalRead++] = c;
				break;
				
			case kScanName:
				if (c == '>')
					fState = kScanText;
				else if (!isalpha(c)) {
					fState = kScanTag;
					name = fCurrentName.String();
					for (int i = 0; i < sizeof(kSourceTags) / sizeof(char*); i++) {
						if (strcasecmp(name, kSourceTags[i]) == 0) {
							// I'm interested in this tag.
							if (c == '=')
								fState = kScanValueStart;
							else
								fState = kScanEqual;

							break;
						}
					}
				} else
					fCurrentName << c;
	
				buf[totalRead++] = c;
				break;

			case kScanEqual:
				if (c == '=')
					fState = kScanValueStart;
					
				buf[totalRead++] = c;
				break;

			case kScanValueStart:
				if (c == '"') {
					fQuoted = true;
					fState = kScanValue;
					buf[totalRead++] = c;
					break;					
				} else if (isspace(c)) {
					fQuoted = false;
					buf[totalRead++] = c;
					break;
				} else
					fQuoted = false;
					
				// Falls through...
				
			case kScanValue:
				if ((fQuoted && c == '"') || (!fQuoted && (c == ' ' || c == '>')))
					ReplaceURL();

				fReplaceString << c;
				continue;
				
			case kReplace:
				buf[totalRead++] = fReplaceString.String()[fURLIndex++];
				if (fURLIndex == fReplaceString.Length()) {
					fState = kScanTag;
					fReplaceString = B_EMPTY_STRING;
				}
			
				break;
		}
	}

	return totalRead;
}

void HTMLMailAdapter::ReplaceURL()
{
	const char *kContentIDURL = "cid:";
	const char *kMerlinURL = "merlin:";
	const char *kBeosURL = "beos:";

	if (memcmp(fReplaceString.String(), kContentIDURL, strlen(kContentIDURL)) == 0) {
		BString tmp;
		tmp = fReplaceString.String() + 4;
		tmp.Prepend("<");
		tmp.Append(">");
		MessagePart *part = fMessage->FindPartByContentID(tmp.String());
		if (!part) {
			// Do something cute
			printf("Oops, couldn't find part %s\n", tmp.String());
		} else {
			// Replace with an internal URL.
			URL newURL("merlin://");
			newURL.AddQueryParameter("action", "1");
			newURL.AddQueryParameter("mailbox", fMessage->GetMailbox());
			newURL.AddQueryParameter("uid", fMessage->GetUid());
			newURL.AddQueryParameter("msgpart", part->id.String());
			fReplaceString.Clear();
			newURL.AppendTo(fReplaceString);
		}
	} else if (memcmp(fReplaceString.String(), kMerlinURL, strlen(kMerlinURL)) == 0
		|| memcmp(fReplaceString.String(), kBeosURL, strlen(kBeosURL)) == 0) {
		printf("Naughty e-mail!  You can't access internal URLs!\n");
		fReplaceString.Clear();
	}

	if (fReplaceString.Length() > 0) {
		fState = kReplace;
		fURLIndex = 0;
	} else
		fState = kScanText;
}


ssize_t HTMLMailAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}
