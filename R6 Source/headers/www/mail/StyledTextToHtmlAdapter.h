/*
	StyledTextToHtmlAdapter.h
*/
#ifndef STYLED_TEXT_TO_ADAPTER
#define STYLED_TEXT_TO_ADAPTER
#include <DataIO.h>
#include <TextView.h>

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

template <class t, int initialCount>
class QuickStack {
public:
	QuickStack()
		:	fSpilloverStack(NULL),
			fSpilloverStackSize(0),
			fStackPos(-1)
	{
	}

	QuickStack(t nullItem)
		:	fSpilloverStack(NULL),
			fSpilloverStackSize(0),
			fStackPos(-1)
	{
		fDummyItem = nullItem;
	}

	~QuickStack()
	{
		if(fSpilloverStack) free(fSpilloverStack);
	}

	int GetSize() { return fStackPos + 1; }

	t ExamineHead()
	{
		if(fStackPos < 0) {
			return fNullItem;
		} else if(fStackPos < initialCount) {
			return fInitialStack[fStackPos];
		} else {
			return fSpilloverStack[fStackPos - initialCount];
		}
	}
	
	int Pop()
	{
		if(fStackPos < 0) {
			return B_ERROR;
		} else  {
			fStackPos--;
			return B_OK;
		}			
	}

	int Push(t item)
	{
		fStackPos++;
		if(fStackPos % initialCount == 0) {
			if(fStackPos - initialCount >= fSpilloverStackSize) {
				t *oldSpilloverStack = fSpilloverStack;
				fSpilloverStack = (t *)realloc(fSpilloverStack, ROUNDUP(fStackPos - initialCount + 1, initialCount) * sizeof(t));
				if(fSpilloverStack == NULL) {
					fSpilloverStack = oldSpilloverStack;
					fStackPos--;
					return B_ERROR;
				}
				fSpilloverStackSize = ROUNDUP(fStackPos - initialCount, initialCount);
			}
		}
		if(fStackPos < initialCount) {
			fInitialStack[fStackPos] = item;
		} else {
			fSpilloverStack[fStackPos - initialCount] = item;
		}
		return B_OK;
	}

	t GetNthItem(int n)
	{
		if(n > fStackPos) {
			return fNullItem;
		} else if(n < initialCount) {
			return fInitialStack[n];
		} else {
			return fSpilloverStack[n - initialCount];
		}
	}

	void Empty() { fStackPos = -1; }

private:
	t fNullItem;
	t fInitialStack[initialCount];
	t *fSpilloverStack;
	int fSpilloverStackSize;
	int fStackPos;
};

class StyledTextToHtmlAdapter : public BDataIO {
	public:
								StyledTextToHtmlAdapter(BDataIO *source, text_run_array *textRun, bool owning = true, bool includeHeaderFooter = true);
		virtual					~StyledTextToHtmlAdapter();
	
		virtual	ssize_t			Read(void *buffer, size_t size);
		virtual ssize_t			Write(const void *buffer, size_t size);
	
	private:
		BDataIO *fSource;
		bool fOwning;
		bool fIncludeHeaderFooter;
		char fInBuffer[4096];
		int fInBufferSize;
		int fInBufferPos;
		unsigned int fTotalBytesPos;

		text_run_array *fTextRun;
		unsigned int fTextRunIndex;
		unsigned int fTextRunPos;
		unsigned int fTextRunSize;

		char fWordBuffer[2048];
		unsigned int fWordBufferPos;
		unsigned int fWordBufferLastPushedPos;

		const char *fReplacementString;
		unsigned int fReplacementStringDumpPos;
		unsigned int fReplacementStringSize;

		const char *fStringToDump;
		unsigned int fStringDumpPos;
		unsigned int fStringSize;
		
		char fColorBuffer[8];

		struct StringFragment {
			union {
				const char *str;
				rgb_color color;
			} u;
			uint16 flags;
			uint16 strLen;
		};
		enum {
			STRING_FRAG_FLAG_NONE = 0,
			STRING_FRAG_FLAG_COLOR,
			STRING_FRAG_FLAG_NOREPLACE
		};

		QuickStack<StringFragment, 64> *fStackToDump;
		StringFragment fStringFrag;
		unsigned int fStringFragPos;
		unsigned int fStringFragIndex;

		QuickStack<int, 16> fStateStack;
		QuickStack<StringFragment, 64> fStringStack;
		QuickStack<StringFragment, 64> fCurrFontStringStack;
};

#endif
