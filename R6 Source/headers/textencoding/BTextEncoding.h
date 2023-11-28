//depot/beos_iad/headers/textencoding/BTextEncoding.h#3 - edit change 51998 (text)
#ifndef _B_TEXT_ENCODING_H_
#define _B_TEXT_ENCODING_H_

#include <support/SupportDefs.h>
#include <support/List.h>
#include <support/String.h>
#include <support/UTF8.h>

namespace B {

namespace TextEncoding {

class conversion_info {
	public:
					conversion_info(bool _unicode = false, uint16 _substitute = B_SUBSTITUTE) :
						state(0L), substitute(_substitute), unicode(_unicode) {}
	int32			state;
	uint16			substitute;
	bool			unicode;
};


class BTextCodec;

class BTextEncoding {
	public:
									BTextEncoding(const char *encodingName = NULL);
									BTextEncoding(uint32 encodingType);
									BTextEncoding(BTextCodec *codec);
									~BTextEncoding();

		status_t					SetTo(const char *encodingName);
		status_t					SetTo(uint32 encodingType);
		
		bool						HasCodec() const;
		const char *				Name() const;

		status_t					ConvertToUnicode(const char *src, int32 *srcLen,
														char *dst, int32 *dstLen,
														conversion_info &info) const;
		status_t					ConvertFromUnicode(const char *src, int32 *srcLen,
														char *dst, int32 *dstLen,
														conversion_info &info) const;
		uint16						UnicodeFor(		uint16 charcode) const;
		uint16						CharcodeFor(	uint16 unicode) const;
		
	private:
		BString						fName;
		BTextCodec *				fCodec;

};



} // end namespace TextEncoding

} // end namespace B


#endif
