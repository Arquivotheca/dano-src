#ifndef _B_TEXT_CODEC_H_
#define _B_TEXT_CODEC_H_

#include <textencoding/BTextEncoding.h>

namespace B {
namespace TextEncoding {

enum text_codec_flags {
	USES_ASCII		= (1 << 0),
	HAS_MULTIBYTE	= (1 << 1),
	ADJUST_VALUE	= (1 << 2),
	ADJUST_FUNCTION	= (1 << 4)
};

typedef const uint16 *const	tcodec_array;

//struct conversion_info {
//	inline conversion_info(uint16 _substitute = B_SUBSTITUTE) : state(0L), substitute(_substitute){};
//	uint32 state;
//	uint16 substitute;
//};

class BTextCodec {
	public:



								BTextCodec(	tcodec_array replacements, const uint16 &replaceCount,
											tcodec_array runs, const uint16 &runCount,
											tcodec_array blocks, tcodec_array blockIndex, const uint16 &blockCount,
											uint32 flags, int32 adjustValue);
		virtual 				~BTextCodec();
		
		/* decodes a native string to utf8 */
		virtual status_t		DecodeString(const char *src, int32 *srcLen, char *dst, int32 *dstLen,
									conversion_info &info) const;
		/* encodes a utf8 string */
		virtual status_t		EncodeString(const char *src, int32 *srcLen, char *dst, int32 *dstLen,
									conversion_info &info) const;
		
		uint16					Decode(uint16 charcode) const;
		uint16					Encode(uint16 unicode) const;

	protected:
		virtual uint16			AdjustCode(uint16 code, bool decode) const;
	private:
		enum encode_array {
			BLOCK = 0,
			REPLACE = 1,
			RUN = 2
		};
		uint16					BinarySearchFor(uint16 charcode, uint8 which);
		uint16					LinearSearchFor(uint16 code, uint8 which, bool decode);

	protected:
		tcodec_array			fReplaces;
		const uint16 &			fReplaceCount;

		tcodec_array			fRuns;
		const uint16 &			fRunCount;

		tcodec_array			fBlocks;
		tcodec_array			fBlockIndex;
		const uint16 &			fBlockCount;
		uint32					fFlags;

	private:
		uint32					fAdjustValue;
		BTextEncodingAddOn *	fAddOn;
		_uint32					_reserved[4];
};


} } // namespace B::TextEncoding

#endif // _TEXT_CODEC_H_