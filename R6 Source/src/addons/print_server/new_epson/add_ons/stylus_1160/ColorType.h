#include <SupportDefs.h>

typedef struct
{
	uint8 c;
	uint8 m;
	uint8 y;
	uint8 k;
} epson_color_t;

typedef uint8 epson_black_t;

#define CONST const

extern CONST epson_color_t gTabColor_360x360_2_plain[8][8][8];
extern CONST epson_color_t gTabColor_360x720_2_plain[8][8][8];
extern CONST epson_color_t gTabColor_720x720_1_plain[8][8][8];
extern CONST epson_color_t gTabColor_360x360_2_inkjet[8][8][8];
extern CONST epson_color_t gTabColor_360x360_2_transparency[8][8][8];
extern CONST epson_color_t gTabColor_360x720_2_inkjet[8][8][8];
extern CONST epson_color_t gTabColor_720x720_1_inkjet[8][8][8];
extern CONST epson_color_t gTabColor_360x720_2_photo[8][8][8];
extern CONST epson_color_t gTabColor_720x720_1_photo[8][8][8];
extern CONST epson_color_t gTabColor_1440x720_0_glossy[8][8][8];



extern       epson_black_t gTabColor_720x720_coated_black[256];

