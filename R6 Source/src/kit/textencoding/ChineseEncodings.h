#include <SupportDefs.h>

#define	EUC_HI_MIN	0xA1
#define	EUC_HI_MAX	0xFE
#define	EUC_LO_MIN	0xA1
#define	EUC_LO_MAX	0xFE
#define	isEucHi(c)	(EUC_HI_MIN <= (c) && (c) <= EUC_HI_MAX)
#define	isEucLo(c)	(EUC_LO_MIN <= (c) && (c) <= EUC_LO_MAX)
#define	DBCS_HI_MIN	0x81
#define	DBCS_HI_MAX	0xFE
#define	DBCS_LO_MIN	0x40
#define	DBCS_LO_MAX	0xFE
#define	isDbcsHi(c)	(DBCS_HI_MIN <= (c) && (c) <= DBCS_HI_MAX)
#define	isDbcsLo(c)	(DBCS_LO_MIN <= (c) && (c) <= DBCS_LO_MAX)
#define	HZ_ENABLED	0x40000000

extern const unsigned short gb2312_to_ucs [][EUC_LO_MAX - EUC_LO_MIN + 1];
extern const unsigned short *ucs_to_gb2312_index [];
extern const unsigned short big5_to_ucs [][DBCS_LO_MAX - DBCS_LO_MIN + 1];
extern const unsigned short *ucs_to_big5_index [];
extern const unsigned short uhc_to_ucs [][DBCS_LO_MAX - DBCS_LO_MIN + 1];
extern const unsigned short *ucs_to_uhc_index [];
extern const unsigned short gbk_to_ucs [][DBCS_LO_MAX - DBCS_LO_MIN + 1];
extern const unsigned short *ucs_to_gbk_index [];

enum { STATE_NORMAL = 0, STATE_DBCS, 
	STATE_ESC_SEQ, STATE_ALT_NORMAL, STATE_ALT_DBCS, STATE_ALT_DBCS_8BIT };

status_t utf8_to_big5(	const char	*src, 
					 	int32			*srcLen, 
					 	char			*dst, 
					 	int32			*dstLen,
					 	int32			*state,
					 	char			substitute);

status_t big5_to_utf8(	const char	*src, 
						int32		*srcLen, 
					  	char		*dst, 
					   	int32		*dstLen,
						int32		*state,
						char		substitute);

status_t gbk_to_utf8(	const char	*src, 
					 	int32			*srcLen, 
					 	char			*dst, 
					 	int32			*dstLen,
					 	int32			*state,
					 	char			substitute);

status_t utf8_to_gbk(	const char	*src, 
						int32		*srcLen, 
					  	char		*dst, 
					   	int32		*dstLen,
						int32		*state,
						char		substitute);


//extern uint16
//	mswin1251tou [0x110],
//	msdos866tou [0x110],
//	mswintou [0x110],
//	msdostou [0x110],
//	macromantou [0x110],
//	iso1tou [0x110],
//	iso2tou [0x110],
//	iso3tou [0x110],
//	iso4tou [0x110],
//	iso5tou [0x110],
//	iso6tou [0x110],
//	iso7tou [0x110],
//	iso8tou [0x110],
//	iso9tou [0x110],
//	iso10tou [0x110],
//	iso13tou [0x110],
//	iso14tou [0x110],
//	iso15tou [0x110],
//	koi8rtou [0x110],
//	sjis00tou [0x110],
//	sjis81tou [0x1f10],
//	sjise0tou [0x1010],
//	EUCKR_SPECIAL [0x470],
//	EUCKR_HANGUL [0x930],
//	EUCKR_HANJA [0x1b18];

//status_t HZ_to_Utf8
//	(uint32 srcEncoding, 
//	const char	*src, 
//	int32		*srcLen, 
//	char		*dst, 
//	int32		*dstLen);
//status_t Utf8_to_HZ
//	(uint32		dstEncoding,
//	const char	*src, 
//	int32		*srcLen, 
//	char		*dst, 
//	int32		*dstLen);
////status_t one_to_utf8 
//	(const uint16 *table, 
//	const char *src, int32 *srclen, 
//	char *dst, int32 *dstLen, 
//	char substitute);
//status_t euc_to_utf8
//	(const char *src, int32 *srclen, 
//	char *dst, int32 *dstLen, 
//	char substitute);
//status_t unicode_to_utf8 
//	(const char *src, int32 *srclen, 
//	char *dst, int32 *dstLen, 
//	char substitute);


//status_t utf8_to_one 
//	(const uint16 *table, 
//	const char *src, int32 *srclen, 
//	char *dst, int32 *dstLen, 
//	char substitute);
//status_t utf8_to_euc 
//	(const char *src, int32 *srclen, 
//	char *dst, int32 *dstLen, 
//	char substitute);
//status_t utf8_to_unicode 
//	(const char *src, int32 *srclen, 
//	char *dst, int32 *dstLen, 
//	char substitute);

