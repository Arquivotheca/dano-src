#include <SupportDefs.h>

typedef struct
{
	uint8 c;
	uint8 m;
	uint8 y;
	uint8 k;
} epson_color_t;

typedef uint8 epson_black_t;

extern const epson_color_t gTabColor_180x180_regular_color[8][8][8];

extern const epson_color_t gTabColor_360x360_regular_color[8][8][8];
extern const epson_color_t gTabColor_360x360_coated_color[8][8][8];
extern const epson_color_t gTabColor_360x360_transparency_color[8][8][8];

extern const epson_color_t gTabColor_360x720_regular_color[8][8][8];
extern const epson_color_t gTabColor_720x720_coated_color[8][8][8];
extern const epson_color_t gTabColor_360x720_glossy_color[8][8][8];

extern epson_black_t gTabColor_720x720_coated_black[256];
