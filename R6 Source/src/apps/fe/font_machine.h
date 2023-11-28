#ifndef	FONT_MACHINE
#define	FONT_MACHINE

#include <stdio.h>
#ifndef _VIEW_H
#include <View.h>
#endif

typedef struct {
	int         offset;
	ushort      code[2];
} fc_hset_item;

typedef struct {
	float       left;
	float       right;
} fc_edge;

typedef struct {
	short       left;           /* bounding box of the string relative */
	short       top;            /* to the current origin point */
	short       right;
	short       bottom;
} fc_rect;

typedef struct {
	float       x_escape;          /* escapement to next character, */
	float       y_escape;          
} fc_escape;

#define CHAR_HEADER_SIZE (sizeof(fc_edge)+sizeof(fc_rect)+sizeof(fc_escape))

typedef struct fc_char {
	fc_edge     edge;              /* edges of the scalable character */
	fc_rect     bbox;              /* bounding box relative to the
									  baseline origin point (bitmap) */
	fc_escape   escape;            /* all the character escapements */
	uchar       bitmap[1];
} fc_char;

class FontMachine {
 public:
	char          *buffer;
	char          pathname[PATH_MAX];
	float         rotation, shear;
	uint16        size;
	uint32        length, hmask, offset, count_char;
	uint32        *list_code;
	fc_char       **list_char;
	bool		  fdirty;
	
	FontMachine();	
	~FontMachine();
	fc_char         *GetChar(uint32 char_num);
	void			RestoreBuffer(uint32 char_num);
	fc_char		    *GetCharBits(uint32 char_num);
	void			SetCharBits(uint32 char_num, fc_char *new_char);
	int				LoadNewFont(char *pathname);
	void            SaveLastFont();
	void            Save();
	void            Apply();
	void			SwapCharFromBig(fc_char *realCh);
	void			SwapCharToBig(fc_char *realCh);
};

#endif

