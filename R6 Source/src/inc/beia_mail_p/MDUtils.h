#ifndef __MD_UTILS_H__
#define __MD_UTILS_H__

class BMallocIO;
class BString;

typedef struct CharsetConversion
{
	char*	name;
	int16	conversion;
} CharsetConversion;

#define B_UTF7_CONVERSION 200
#define B_UTF8_CONVERSION 201

#ifdef __cplusplus
extern "C" {
#endif

extern "C" _EXPORT int16 charset_to_utf8const(const char *);
extern "C" _EXPORT const char *utf8const_to_charset(int16);

extern "C" _EXPORT void ConvertDataToUTF8(BMallocIO *,int16);

extern "C" _EXPORT bool base64_decode_string(BString *);
extern "C" _EXPORT bool base64_encode_string(BString *);

#ifdef __cplusplus
}
#endif

extern const struct CharsetConversion kCharsets[];
extern const uint32 kNumCharsets;

#endif
