#ifndef _FONT_UTILS_H_
#define _FONT_UTILS_H_

#include <SupportDefs.h>
#include <hash_map>

enum {
	UNKNOWN_FONT			= -1,
	HELVETICA				= 0,
	HELVETICA_BOLD			= 1,
	HELVETICA_OBLIQUE		= 2,
	HELVETICA_BOLDOBLIQUE	= 3,
	TIMES_ROMAN				= 4,
	TIMES_ROMAN_BOLD	 	= 5,
	TIMES_ROMAN_ITALIC		= 6,
	TIMES_ROMAN_BOLDITALIC	= 7,
	SYMBOL					= 8,
	ZAPF_DINGBATS			= 9,
	COURIER					= 10,
	COURIER_BOLD			= 11,
	COURIER_OBLIQUE			= 12,
	COURIER_BOLDOBLIQUE		= 13
};

enum {
	UNKNOWN_ENCODING = -1,
	STD_ADOBE_ENCODING = 0,
	MAC_ROMAN_ENCODING = 1,
	WIN_ANSI_ENCODING = 2,
	MAC_EXPERT_ENCODING = 3,
	SYMBOL_ENCODING = 4,
	ZAPF_DINGBATS_ENCODING = 5		
};



class CodeToPSNameMap {
	public:
								CodeToPSNameMap();
								~CodeToPSNameMap();
								
		inline const char *		NameForCode(uint16 code, int8 encoding) {
			if (encoding < 0 || encoding > 5)
				return NULL;
			code -=32;
			if (code > 223)
				return NULL;
			else
				return fNames[encoding][code];
		};
	private:
		void					init_name_in_encodings(const char *name, uint16 a, uint16 r, uint16 w, uint16 e, uint16 s, uint16 d);
		typedef	const char *	encoding_info[224];
		encoding_info			fNames[6];
		
};

struct str_equal_to {
	bool operator()(const char *a, const char *b)
		{	return (::strcmp(a, b) == 0); };
};

class StandardWidths {
	public:
						StandardWidths();
						~StandardWidths();
		float			WidthForName(const char *name, int8 font);
		
	private:

		struct width_info {
			width_info() { memset(widths, 0, sizeof(uint16) * 8);}
			width_info(uint16 h, uint16 hb, uint16 ho, uint16 hbo,
						uint16 t, uint16 tb, uint16 ti, uint16 tbi) {
				widths[HELVETICA] = h;
				widths[HELVETICA_BOLD] = hb;
				widths[HELVETICA_OBLIQUE] = ho;
				widths[HELVETICA_BOLDOBLIQUE] = hbo;
				widths[TIMES_ROMAN] = t;
				widths[TIMES_ROMAN_BOLD] = tb;
				widths[TIMES_ROMAN_ITALIC] = ti;
				widths[TIMES_ROMAN_BOLDITALIC] = tbi;
			};
			uint16 widths[8];
		};
		typedef hash_map<const char *, width_info, hash<const char *>, str_equal_to> std_width_map;
		
		
		typedef hash_map<const char *, uint16, hash<const char *>, str_equal_to> spec_width_map;
		
//		typedef hash_map<const char *, uint16, hash<const char *>, str_equal_to> spec_width_map;

		
		void			init_name_in_widths(const char *, width_info );
		void			init_name_in_spec(const char *, uint16, bool);
		
		std_width_map	fStdWidths;
		
		spec_width_map	fSymbolWidths;
		
		spec_width_map	fDingbatWidths;
};

class PSNameToUnicodeMap {
	public:
								PSNameToUnicodeMap();
								~PSNameToUnicodeMap();
		uint16					UnicodeFor(const char *name);
	private:
		typedef hash_map<const char *, uint16, hash<const char *>, str_equal_to> unicode_map;
		unicode_map				fCodes;
		void					init_name_in_codes(const char *, uint16);
};

#endif
