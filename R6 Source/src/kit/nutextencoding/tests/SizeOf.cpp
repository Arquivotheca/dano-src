
#include <stdio.h>
//#include </source/iad/headers/add-ons/textencoding/BEncodingRun.h>

#include <SupportDefs.h>

//using namespace B::TextEncoding;

#include <SupportDefs.h>

class encoding_run {
	encoding_run(uint8 type);
//	uint8 type;
};

class replacement_run : public encoding_run{
	replacement_run(uint16 c, uint16 u);
	uint16 charcode;
	uint16 unicode;
};

class sequential_run : public encoding_run {
	sequential_run(uint16 cS, uint16 cE, uint16 uS);
	uint16 charcodeStart;
	uint16 charcodeEnd;
	uint16 unicodeStart;
};

class constant_block_run : public encoding_run {
	constant_block_run(uint16 cS, uint16 cE, const uint16 *block);
	const uint16 *b;
	uint16 cS, cE;
};

class block_run : public encoding_run {
	block_run(uint16 cS, uint16 cE, uint16 * block, bool fB);
	uint16 *b;
	uint16 cS, cE;
};

struct replace {
	uint16 a;
};
struct sequence {
	uint16 a;
};
struct cblock {
	const uint16 *c;
};
struct block {
	uint16 * a;
};

class test {
	test();
	~test();
	union {
		replace r;
		sequence s;
		cblock cb;
		block b;
	};
	uint16 cS; uint16 cE;
	uint8 type;
};

struct rep {
			rep(uint16 _a, uint16 _b) : a(_a), b(_b) {}
uint16 a, b;
};

extern "C" {
struct b_block {
	uint16 start, end;
	const uint16 * block;
};
}

class b_sequence {
	b_sequence();
	uint16 cfor(uint16 u);
	uint16 ufor(uint16 c);
	uint16 start, end, ustart;
};


const uint16	GBKBlock_8140[] = {
0x4E02, 0x4E04, 0x4E05, 0x4E06, 0x4E0F, 0x4E12, 0x4E17, 0x4E1F, 0x4E20, 0x4E21, 0x4E23, 0x4E26, 0x4E29, 0x4E2E, 0x4E2F, 0x4E31,
0x4E33, 0x4E35, 0x4E37, 0x4E3C, 0x4E40, 0x4E41, 0x4E42, 0x4E44, 0x4E46, 0x4E4A, 0x4E51, 0x4E55, 0x4E57, 0x4E5A, 0x4E5B, 0x4E62,
0x4E63, 0x4E64, 0x4E65, 0x4E67, 0x4E68, 0x4E6A, 0x4E6B, 0x4E6C, 0x4E6D, 0x4E6E, 0x4E6F, 0x4E72, 0x4E74, 0x4E75, 0x4E76, 0x4E77,
0x4E78, 0x4E79, 0x4E7A, 0x4E7B, 0x4E7C, 0x4E7D, 0x4E7F, 0x4E80, 0x4E81, 0x4E82, 0x4E83, 0x4E84, 0x4E85, 0x4E87, 0x4E8A
};
const b_block		GBK_8140 = {0x8140, 0x817E, GBKBlock_8140};
const b_block * const GBKStack[] = {
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140, &GBK_8140,
&GBK_8140
};
const uint16 GBKStackSize = 271;

class BEncodingRun;
class BReplacement;
class BTextEncodingAddOn;
class BTextCodec {
	public:
									BTextCodec(const BEncodingRun * const *array, uint16 arraySize, const BReplacement * const *replacements, uint16 replacementCount, BTextEncodingAddOn *addOn, uint32 flags = 0, int32 adjustValue = 0);	
		virtual						~BTextCodec();

//		virtual status_t			ConvertToUnicode(const char *src, int32 *srcLen,
//														char *dst, int32 *dstLen,
//														conversion_info &info, BEncodingStack *stack) const;
//		virtual status_t			ConvertFromUnicode(const char *src, int32 *srcLen,
//														char *dst, int32 *dstLen,
//														conversion_info &info, BEncodingStack *stack) const;
		uint16						UnicodeFor(uint16 charcode) const;
		uint16						CharcodeFor(uint16 unicode) const;
	
	protected:
		virtual uint16				AdjustCharcode(uint16 charcode, bool convert_to = true) const;

	private:
		uint16						BinarySearchFor(uint16 charcode, bool searchReplacements) const;
		uint16						LinearSearchFor(uint16 charcode, bool searchReplacements) const;

		BTextEncodingAddOn *		fAddOn;
		int32						fAdjustValue;

	protected:
		const BEncodingRun * const *fArray;
		const BReplacement * const *fReplacements;
		uint32						fFlags;
		uint16						fArraySize;
		uint16						fReplacementCount;
								
};


int main () {
	printf("BTextCodec: %ld\n", sizeof(BTextCodec));


//	printf("BSequentialRun: %ld BReplacement: %ld B\n",
//		sizeof(BEncodingRun), sizeof(BReplacement));

	printf("b_block: %ld b_sequence: %ld\n", sizeof(b_block), sizeof(b_sequence));

	printf("r: %ld, s: %ld cb: %ld b: %ld\n", sizeof(replace), sizeof(sequence), sizeof(cblock), sizeof(block));
	printf("test: %ld\n", sizeof(test));
	
	printf("er: %ld rr: %ld sr: %ld cbr: %ld br: %ld\n",
		sizeof(encoding_run), sizeof(replacement_run), sizeof(sequential_run), sizeof(constant_block_run), sizeof(block_run));

	printf("rep: %ld\n", sizeof(rep));
}
