
#include <support/UTF8.h>
#include <textencoding/BTextEncoding.h>
#include <textencoding/TextEncodingNames.h>
#include <Locker.h>

using namespace B::TextEncoding;

BTextEncoding gEncoding;
BLocker gEncodeLock;


status_t 
convert_to_utf8(uint32 srcEncoding, const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute)
{
	status_t status = B_OK;
	if (gEncodeLock.Lock()) {
		
		status = gEncoding.SetTo(srcEncoding);
		if (status != B_OK)
			return status;
		
		conversion_info info;
		if (state)
			info.state = *state;
		info.substitute = substitute;
		
		status = gEncoding.ConvertToUnicode(src, srcLen, dst, dstLen, info);
		if (state)
			*state = info.state;
		gEncodeLock.Unlock();
	}
	else
		status = B_ERROR;
	return status;
}

status_t 
convert_from_utf8(uint32 dstEncoding, const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute)
{
	status_t status = B_OK;
	if (gEncodeLock.Lock()) {
		
		status = gEncoding.SetTo(dstEncoding);
		if (status != B_OK)
			return status;
		
		conversion_info info;
		if (state)
			info.state = *state;
		info.substitute = substitute;
		
		status = gEncoding.ConvertFromUnicode(src, srcLen, dst, dstLen, info);
		if (state)
			*state = info.state;
		gEncodeLock.Unlock();
	}
	else
		status = B_ERROR;
	return status;
}



const char *
name_for_encoding_enum(uint32 type)
{
	switch (type) {
		case B_ISO1_CONVERSION:
			return ISO1Name;
		case B_ISO2_CONVERSION:
			return ISO2Name;
		case B_ISO3_CONVERSION:
			return ISO3Name;
		case B_ISO4_CONVERSION:
			return ISO4Name;
		case B_ISO5_CONVERSION:
			return ISO5Name;
		case B_ISO6_CONVERSION:
			return ISO6Name;
		case B_ISO7_CONVERSION:
			return ISO7Name;
		case B_ISO8_CONVERSION:
			return ISO8Name;
		case B_ISO9_CONVERSION:
			return ISO9Name;
		case B_ISO10_CONVERSION:
			return ISO10Name;
		case B_ISO13_CONVERSION:
			return ISO13Name;
		case B_ISO14_CONVERSION:
			return ISO14Name;
		case B_ISO15_CONVERSION:
			return ISO15Name;
		case B_UNICODE_CONVERSION:
			return UnicodeName;
		case B_SJIS_CONVERSION:
			return SJISName;
		case B_EUC_CONVERSION:
			return EUCName;
		case B_JIS_CONVERSION:
			return JISName;
		case B_KOI8R_CONVERSION:
			return KOI8RName;
		case B_EUC_KR_CONVERSION:
			return EUCKRName;
		case B_BIG5_CONVERSION:
			return Big5Name;
		case B_GBK_CONVERSION:
			return GBKName;
		case B_MS_WINDOWS_CONVERSION:
			return MSWinName;
		case B_MS_WINDOWS_1250_CONVERSION:
			return CP1250Name;
		case B_MS_WINDOWS_1251_CONVERSION:
			return CP1251Name;
		case B_MS_WINDOWS_1253_CONVERSION:
			return CP1253Name;
		case B_MS_WINDOWS_1254_CONVERSION:
			return CP1254Name;
		case B_MS_WINDOWS_1255_CONVERSION:
			return CP1255Name;
		case B_MS_WINDOWS_1256_CONVERSION:
			return CP1256Name;
		case B_MS_WINDOWS_1257_CONVERSION:
			return CP1257Name;
		case B_MS_WINDOWS_1258_CONVERSION:
			return CP1258Name;
		case B_MS_DOS_CONVERSION:
			return MSDosName;
		case B_MS_DOS_737_CONVERSION:
			return CP737Name;
		case B_MS_DOS_775_CONVERSION:
			return CP775Name;
		case B_MS_DOS_850_CONVERSION:
			return CP850Name;
		case B_MS_DOS_852_CONVERSION:
			return CP852Name;
		case B_MS_DOS_855_CONVERSION:
			return CP855Name;
		case B_MS_DOS_857_CONVERSION:
			return CP857Name;
		case B_MS_DOS_860_CONVERSION:
			return CP860Name;
		case B_MS_DOS_861_CONVERSION:
			return CP861Name;
		case B_MS_DOS_862_CONVERSION:
			return CP862Name;
		case B_MS_DOS_863_CONVERSION:
			return CP863Name;
		case B_MS_DOS_864_CONVERSION:
			return CP864Name;
		case B_MS_DOS_865_CONVERSION:
			return CP865Name;
		case B_MS_DOS_866_CONVERSION:
			return CP866Name;
		case B_MS_DOS_869_CONVERSION:
			return CP869Name;
		case B_MS_DOS_874_CONVERSION:
			return CP874Name;	
		case B_MAC_ROMAN_CONVERSION:
			return MacRomanName;
		case B_MAC_CENTEURO_CONVERSION:
			return MacCenteuroName;
		case B_MAC_CROATIAN_CONVERSION:
			return MacCroatianName;
		case B_MAC_CYRILLIC_CONVERSION:
			return MacCyrillicName;
		case B_MAC_GREEK_CONVERSION:
			return MacGreekName;
		case B_MAC_HEBREW_CONVERSION:
			return MacHebrewName;
		case B_MAC_ICELAND_CONVERSION:
			return MacIcelandName;
		case B_MAC_TURKISH_CONVERSION:
			return MacTurkishName;
		case B_MAC_EXPERT_CONVERSION:
			return MacExpertName;

		case B_NO_CONVERSION:
		default:
			return NULL;
	}
}

