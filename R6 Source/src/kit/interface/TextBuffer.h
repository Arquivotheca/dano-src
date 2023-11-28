/* ++++++++++

   FILE:  TextBuffer.h

+++++ */

#ifndef _TEXT_BUFFER_H
#define _TEXT_BUFFER_H

#ifndef _OBJECT_H
#include <Object.h>
#endif

class BView;

class _BTextBuffer_ : public BObject {
public:
					_BTextBuffer_(long allocSize, long gapSize);
virtual				~_BTextBuffer_();

		const uchar	&operator[](long index);

		void		Set(const char *string);
		void		Set(const char *data, long length);
		void		Insert(long index, const char *string);
		void		Insert(long index, const char *data, long length);
		void		Delete(long index, long length);
		void		DrawString(long index, long length, BView *view) const;
		const char	*Text();
		long		TextLength() const;
		char		*GetText(char *buffer, long index, long length) const;
		void		PrintToStream() const;

private:
		void		CollapseGap();
		void		Reset();

		char		*fBuffer;
		long		fLogicalSize;
		long		fPhysicalSize;
		long		fGapStart;
		long		fGapEnd;
		long		fBlockSize;
		long		fInitGapSize;
};

inline long _BTextBuffer_::TextLength() const
	{ return fLogicalSize ; };

#endif
