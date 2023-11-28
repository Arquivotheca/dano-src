#include <SupportDefs.h>

typedef struct
{
	uint8 c;
	uint8 m;
	uint8 y;
	uint8 k;
	uint8 lc;
	uint8 lm;
} epson_color_t;

typedef uint8 epson_black_t;

extern const epson_color_t gTabColor_720x360_coated_color[8][8][8];
extern const epson_color_t gTabColor_360x720_regular_color[8][8][8];
extern const epson_color_t gTabColor_720x360_regular_color[8][8][8];
extern const epson_color_t (*gTabColor_360x720_coated_color)[8][8];
extern const epson_color_t gTabColor_360x360_transparency_color[8][8][8];
extern const epson_color_t gTabColor_720x720_coated_color[8][8][8];
extern const epson_color_t gTabColor_720x720_photo_color[8][8][8];
extern const epson_color_t gTabColor_1440x720_coated_color[8][8][8];
extern const epson_color_t gTabColor_1440x720_photo_color[8][8][8];
extern const epson_color_t gTabColor_1440x720_film_color[8][8][8];
extern epson_black_t gTabColor_720x720_coated_black[256];
