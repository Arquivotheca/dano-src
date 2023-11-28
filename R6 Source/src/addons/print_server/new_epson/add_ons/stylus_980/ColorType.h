#include <SupportDefs.h>

#define CONST const

typedef struct
{
	uint8 c;
	uint8 m;
	uint8 y;
	uint8 k;
} epson_color_t;

typedef uint8 epson_black_t;

extern const epson_color_t gTabColor_360x360_01_plain[8][8][8];
extern const epson_color_t gTabColor_360x360_11_plain[8][8][8];
extern const epson_color_t gTabColor_720x720_10_plain[8][8][8];
extern const epson_color_t gTabColor_360x360_11_inkjet360[8][8][8];
extern const epson_color_t gTabColor_360x720_11_inkjet[8][8][8];
extern const epson_color_t gTabColor_720x720_10_inkjet[8][8][8];
extern const epson_color_t gTabColor_360x720_11_photo[8][8][8];
extern const epson_color_t gTabColor_720x720_10_photo[8][8][8];
extern const epson_color_t gTabColor_1440x720_10_photo[8][8][8];
extern const epson_color_t gTabColor_1440x720_10_glossy[8][8][8];
extern const epson_color_t gTabColor_360x360_01_transparency[8][8][8];

extern epson_black_t gTabColor_720x720_coated_black[256];

