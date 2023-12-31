#include "ColorType.h"

const epson_color_t gTabColor_720x720_photo_color[8][8][8] = 
{
	{
		{
			{  0,   0,   0, 203,   0,   0},	 //   0,  0,  0
			{ 67, 131,  25,  67,   0,   0},	 //   0,  0,  1
			{127, 184,  43,  17,   0,   0},	 //   0,  0,  2
			{203, 170,  13,   1,   0,   1},	 //   0,  0,  3
			{202, 100,   1,   0,   0,  64},	 //   0,  0,  4
			{151,  64,   0,   0,  25, 119},	 //   0,  0,  5
			{ 82,  45,   0,   0, 117, 140},	 //   0,  0,  6
			{ 47,  35,   0,   0, 166, 150},	 //   0,  0,  7
		},
		{
			{ 74,  58, 178,  21,   0,   2},	 //   0,  1,  0
			{126,  77,  82,   3,  23,  26},	 //   0,  1,  1
			{140,  56,  40,   0,  33,  99},	 //   0,  1,  2
			{147,  33,  11,   0,  35, 123},	 //   0,  1,  3
			{163,  15,   0,   0,  24, 138},	 //   0,  1,  4
			{ 77,  10,   0,   0, 126, 162},	 //   0,  1,  5
			{ 37,   8,   0,   0, 180, 163},	 //   0,  1,  6
			{ 18,   8,   0,   0, 196, 162},	 //   0,  1,  7
		},
		{
			{ 82,  16, 204,   0,  55,  51},	 //   0,  2,  0
			{ 56,   6, 126,   0, 134,  77},	 //   0,  2,  1
			{ 22,   4,  41,   0, 193, 104},	 //   0,  2,  2
			{ 25,   1,  24,   0, 191, 104},	 //   0,  2,  3
			{ 27,   0,  12,   0, 190, 102},	 //   0,  2,  4
			{ 32,   0,   0,   0, 183,  95},	 //   0,  2,  5
			{ 12,   0,   0,   0, 199, 113},	 //   0,  2,  6
			{  1,   0,   0,   0, 201, 128},	 //   0,  2,  7
		},
		{
			{ 36,   3, 198,   0, 156,   7},	 //   0,  3,  0
			{ 10,   0, 107,   0, 192,  34},	 //   0,  3,  1
			{  1,   0,  54,   0, 197,  50},	 //   0,  3,  2
			{  0,   0,  22,   0, 193,  62},	 //   0,  3,  3
			{  0,   0,  15,   0, 194,  56},	 //   0,  3,  4
			{  1,   0,   7,   0, 199,  53},	 //   0,  3,  5
			{  0,   0,   0,   0, 198,  56},	 //   0,  3,  6
			{  0,   0,   0,   0, 181,  78},	 //   0,  3,  7
		},
		{
			{  4,   0, 134,   0, 180,   0},	 //   0,  4,  0
			{  1,   0,  91,   0, 179,   6},	 //   0,  4,  1
			{  0,   0,  53,   0, 172,  15},	 //   0,  4,  2
			{  0,   0,  34,   0, 171,  19},	 //   0,  4,  3
			{  0,   0,  10,   0, 166,  26},	 //   0,  4,  4
			{  0,   0,   9,   0, 166,  24},	 //   0,  4,  5
			{  0,   0,   2,   0, 172,  22},	 //   0,  4,  6
			{  0,   0,   0,   0, 164,  34},	 //   0,  4,  7
		},
		{
			{  0,   0,  96,   0, 160,   0},	 //   0,  5,  0
			{  0,   0,  78,   0, 156,   0},	 //   0,  5,  1
			{  0,   0,  51,   0, 151,   0},	 //   0,  5,  2
			{  0,   0,  33,   0, 150,   0},	 //   0,  5,  3
			{  0,   0,  21,   0, 151,   0},	 //   0,  5,  4
			{  0,   0,   5,   0, 145,   4},	 //   0,  5,  5
			{  0,   0,   3,   0, 145,   4},	 //   0,  5,  6
			{  0,   0,   0,   0, 148,   7},	 //   0,  5,  7
		},
		{
			{  0,   0,  82,   0, 141,   0},	 //   0,  6,  0
			{  0,   0,  72,   0, 138,   0},	 //   0,  6,  1
			{  0,   0,  53,   0, 135,   0},	 //   0,  6,  2
			{  0,   0,  41,   0, 132,   0},	 //   0,  6,  3
			{  0,   0,  26,   0, 132,   0},	 //   0,  6,  4
			{  0,   0,  15,   0, 131,   0},	 //   0,  6,  5
			{  0,   0,   4,   0, 127,   1},	 //   0,  6,  6
			{  0,   0,   1,   0, 126,   0},	 //   0,  6,  7
		},
		{
			{  0,   0,  73,   0, 126,   0},	 //   0,  7,  0
			{  0,   0,  66,   0, 125,   0},	 //   0,  7,  1
			{  0,   0,  51,   0, 120,   0},	 //   0,  7,  2
			{  0,   0,  42,   0, 120,   0},	 //   0,  7,  3
			{  0,   0,  33,   0, 117,   0},	 //   0,  7,  4
			{  0,   0,  24,   0, 117,   0},	 //   0,  7,  5
			{  0,   0,  15,   0, 118,   0},	 //   0,  7,  6
			{  0,   0,   3,   0, 112,   0},	 //   0,  7,  7
		},
	},
	{
		{
			{ 35, 115, 142,  39,   0,   0},	 //   1,  0,  0
			{ 55, 169, 100,  18,   3,   0},	 //   1,  0,  1
			{ 62, 204,  27,   0,  83,   0},	 //   1,  0,  2
			{ 50, 166,   0,   0, 128,  10},	 //   1,  0,  3
			{ 49,  79,   0,   0, 134, 122},	 //   1,  0,  4
			{ 44,  48,   1,   0, 140, 161},	 //   1,  0,  5
			{ 35,  35,   0,   0, 149, 167},	 //   1,  0,  6
			{ 25,  26,   0,   0, 160, 172},	 //   1,  0,  7
		},
		{
			{ 61,  84, 189,   8,   5,   8},	 //   1,  1,  0
			{ 72,  93, 105,   0,  24,  54},	 //   1,  1,  1
			{ 42,  48,  38,   0, 117, 153},	 //   1,  1,  2
			{ 32,  34,  11,   0, 151, 161},	 //   1,  1,  3
			{ 23,  25,   4,   0, 172, 160},	 //   1,  1,  4
			{ 17,  14,   1,   0, 181, 167},	 //   1,  1,  5
			{ 11,  12,   0,   0, 183, 165},	 //   1,  1,  6
			{  8,   8,   0,   0, 183, 164},	 //   1,  1,  7
		},
		{
			{ 41,  19, 198,   0,  81,  64},	 //   1,  2,  0
			{ 20,   6, 119,   0, 134, 100},	 //   1,  2,  1
			{  5,   5,  46,   0, 163, 123},	 //   1,  2,  2
			{  2,   1,  24,   0, 176, 128},	 //   1,  2,  3
			{  0,   0,  10,   0, 188, 124},	 //   1,  2,  4
			{  0,   0,   2,   0, 188, 123},	 //   1,  2,  5
			{  0,   0,   0,   0, 182, 128},	 //   1,  2,  6
			{  0,   0,   0,   0, 178, 133},	 //   1,  2,  7
		},
		{
			{  8,   0, 181,   0, 157,  32},	 //   1,  3,  0
			{  1,   0, 104,   0, 165,  52},	 //   1,  3,  1
			{  0,   0,  53,   0, 161,  63},	 //   1,  3,  2
			{  0,   0,  24,   0, 157,  70},	 //   1,  3,  3
			{  0,   0,  13,   0, 160,  67},	 //   1,  3,  4
			{  0,   0,   6,   0, 163,  66},	 //   1,  3,  5
			{  0,   0,   1,   0, 161,  71},	 //   1,  3,  6
			{  0,   0,   0,   0, 159,  85},	 //   1,  3,  7
		},
		{
			{  0,   0, 135,   0, 159,   4},	 //   1,  4,  0
			{  0,   0,  82,   0, 151,  15},	 //   1,  4,  1
			{  0,   0,  48,   0, 144,  21},	 //   1,  4,  2
			{  0,   0,  30,   0, 142,  26},	 //   1,  4,  3
			{  0,   0,  11,   0, 139,  31},	 //   1,  4,  4
			{  0,   0,   6,   0, 139,  30},	 //   1,  4,  5
			{  0,   0,   2,   0, 145,  30},	 //   1,  4,  6
			{  0,   0,   0,   0, 145,  40},	 //   1,  4,  7
		},
		{
			{  0,   0,  97,   0, 141,   0},	 //   1,  5,  0
			{  0,   0,  76,   0, 135,   6},	 //   1,  5,  1
			{  0,   0,  48,   0, 129,   8},	 //   1,  5,  2
			{  0,   0,  33,   0, 128,   8},	 //   1,  5,  3
			{  0,   0,  21,   0, 128,   9},	 //   1,  5,  4
			{  0,   0,   6,   0, 125,  11},	 //   1,  5,  5
			{  0,   0,   3,   0, 125,   9},	 //   1,  5,  6
			{  0,   0,   0,   0, 132,  11},	 //   1,  5,  7
		},
		{
			{  0,   0,  83,   0, 127,   0},	 //   1,  6,  0
			{  0,   0,  71,   0, 121,   2},	 //   1,  6,  1
			{  0,   0,  53,   0, 119,   2},	 //   1,  6,  2
			{  0,   0,  37,   0, 116,   2},	 //   1,  6,  3
			{  0,   0,  25,   0, 114,   3},	 //   1,  6,  4
			{  0,   0,  15,   0, 116,   3},	 //   1,  6,  5
			{  0,   0,   3,   0, 111,   0},	 //   1,  6,  6
			{  0,   0,   0,   0, 111,   0},	 //   1,  6,  7
		},
		{
			{  0,   0,  73,   0, 115,   0},	 //   1,  7,  0
			{  0,   0,  65,   0, 114,   0},	 //   1,  7,  1
			{  0,   0,  51,   0, 110,   0},	 //   1,  7,  2
			{  0,   0,  42,   0, 108,   0},	 //   1,  7,  3
			{  0,   0,  33,   0, 107,   0},	 //   1,  7,  4
			{  0,   0,  24,   0, 107,   0},	 //   1,  7,  5
			{  0,   0,  15,   0, 108,   0},	 //   1,  7,  6
			{  0,   0,   3,   0, 103,   0},	 //   1,  7,  7
		},
	},
	{
		{
			{ 40, 167, 170,   4,  16,   0},	 //   2,  0,  0
			{ 23, 201, 100,   0,  66,   0},	 //   2,  0,  1
			{  3, 195,  50,   0, 101,   0},	 //   2,  0,  2
			{  3, 154,   0,   0, 119,  19},	 //   2,  0,  3
			{  1,  78,   0,   0, 141, 122},	 //   2,  0,  4
			{  1,  40,   0,   0, 149, 177},	 //   2,  0,  5
			{  1,  22,   0,   0, 151, 195},	 //   2,  0,  6
			{  1,  11,   0,   0, 151, 201},	 //   2,  0,  7
		},
		{
			{ 19,  74, 176,   0,  68,  62},	 //   2,  1,  0
			{  8,  58,  99,   0,  94, 137},	 //   2,  1,  1
			{  1,  36,  46,   0, 104, 179},	 //   2,  1,  2
			{  0,  18,  11,   0, 120, 195},	 //   2,  1,  3
			{  0,  14,   4,   0, 132, 194},	 //   2,  1,  4
			{  0,   7,   1,   0, 139, 198},	 //   2,  1,  5
			{  0,   0,   0,   0, 146, 199},	 //   2,  1,  6
			{  0,   0,   0,   0, 149, 191},	 //   2,  1,  7
		},
		{
			{  8,  14, 182,   0,  93,  98},	 //   2,  2,  0
			{  2,   5, 109,   0, 104, 135},	 //   2,  2,  1
			{  1,   2,  44,   0, 115, 151},	 //   2,  2,  2
			{  0,   0,  24,   0, 116, 150},	 //   2,  2,  3
			{  0,   0,   8,   0, 125, 136},	 //   2,  2,  4
			{  0,   0,   2,   0, 129, 133},	 //   2,  2,  5
			{  0,   0,   0,   0, 137, 139},	 //   2,  2,  6
			{  0,   0,   0,   0, 143, 141},	 //   2,  2,  7
		},
		{
			{  0,   0, 154,   0, 114,  53},	 //   2,  3,  0
			{  0,   0,  93,   0, 114,  69},	 //   2,  3,  1
			{  0,   0,  52,   0, 108,  78},	 //   2,  3,  2
			{  0,   0,  28,   0, 109,  83},	 //   2,  3,  3
			{  0,   0,  15,   0, 111,  78},	 //   2,  3,  4
			{  0,   0,   6,   0, 114,  75},	 //   2,  3,  5
			{  0,   0,   1,   0, 119,  79},	 //   2,  3,  6
			{  0,   0,   0,   0, 125,  89},	 //   2,  3,  7
		},
		{
			{  0,   0, 118,   0, 112,  19},	 //   2,  4,  0
			{  0,   0,  78,   0, 107,  28},	 //   2,  4,  1
			{  0,   0,  47,   0, 105,  33},	 //   2,  4,  2
			{  0,   0,  32,   0, 103,  37},	 //   2,  4,  3
			{  0,   0,  15,   0, 100,  40},	 //   2,  4,  4
			{  0,   0,   9,   0, 105,  38},	 //   2,  4,  5
			{  0,   0,   2,   0, 106,  39},	 //   2,  4,  6
			{  0,   0,   0,   0, 110,  52},	 //   2,  4,  7
		},
		{
			{  0,   0,  93,   0, 105,   0},	 //   2,  5,  0
			{  0,   0,  70,   0, 103,   9},	 //   2,  5,  1
			{  0,   0,  47,   0, 100,  11},	 //   2,  5,  2
			{  0,   0,  33,   0,  99,  12},	 //   2,  5,  3
			{  0,   0,  22,   0,  98,  13},	 //   2,  5,  4
			{  0,   0,   7,   0,  96,  15},	 //   2,  5,  5
			{  0,   0,   3,   0,  96,  14},	 //   2,  5,  6
			{  0,   0,   0,   0, 101,  19},	 //   2,  5,  7
		},
		{
			{  0,   0,  83,   0, 100,   0},	 //   2,  6,  0
			{  0,   0,  69,   0,  98,   2},	 //   2,  6,  1
			{  0,   0,  49,   0,  96,   3},	 //   2,  6,  2
			{  0,   0,  36,   0,  94,   3},	 //   2,  6,  3
			{  0,   0,  25,   0,  94,   4},	 //   2,  6,  4
			{  0,   0,  15,   0,  94,   4},	 //   2,  6,  5
			{  0,   0,   3,   0,  92,   1},	 //   2,  6,  6
			{  0,   0,   0,   0,  92,   1},	 //   2,  6,  7
		},
		{
			{  0,   0,  73,   0,  96,   0},	 //   2,  7,  0
			{  0,   0,  65,   0,  96,   0},	 //   2,  7,  1
			{  0,   0,  51,   0,  93,   0},	 //   2,  7,  2
			{  0,   0,  42,   0,  93,   0},	 //   2,  7,  3
			{  0,   0,  33,   0,  92,   0},	 //   2,  7,  4
			{  0,   0,  24,   0,  91,   0},	 //   2,  7,  5
			{  0,   0,  14,   0,  91,   0},	 //   2,  7,  6
			{  0,   0,   2,   0,  87,   0},	 //   2,  7,  7
		},
	},
	{
		{
			{  7, 171, 163,   0,  55,   0},	 //   3,  0,  0
			{  0, 201,  67,   0,  67,   0},	 //   3,  0,  1
			{  0, 185,  40,   0,  67,   0},	 //   3,  0,  2
			{  1, 169,  21,   0,  62,   6},	 //   3,  0,  3
			{  0,  62,   0,   0,  85, 147},	 //   3,  0,  4
			{  0,  27,   0,   0, 100, 189},	 //   3,  0,  5
			{  0,  16,   0,   0, 109, 196},	 //   3,  0,  6
			{  0,  10,   0,   0, 116, 201},	 //   3,  0,  7
		},
		{
			{  8,  87, 181,   0,  56,  73},	 //   3,  1,  0
			{  0,  51,  90,   0,  69, 155},	 //   3,  1,  1
			{  0,  25,  41,   0,  68, 190},	 //   3,  1,  2
			{  0,  23,  24,   0,  67, 192},	 //   3,  1,  3
			{  0,   8,   4,   0,  82, 200},	 //   3,  1,  4
			{  0,   7,   1,   0,  94, 200},	 //   3,  1,  5
			{  0,   1,   0,   0, 100, 200},	 //   3,  1,  6
			{  0,   0,   0,   0, 108, 198},	 //   3,  1,  7
		},
		{
			{  0,   5, 180,   0,  74, 121},	 //   3,  2,  0
			{  0,   1,  94,   0,  73, 151},	 //   3,  2,  1
			{  0,   0,  46,   0,  72, 163},	 //   3,  2,  2
			{  0,   0,  28,   0,  70, 163},	 //   3,  2,  3
			{  0,   0,   8,   0,  75, 150},	 //   3,  2,  4
			{  0,   0,   2,   0,  83, 147},	 //   3,  2,  5
			{  0,   0,   0,   0,  90, 148},	 //   3,  2,  6
			{  0,   0,   0,   0,  99, 154},	 //   3,  2,  7
		},
		{
			{  0,   0, 138,   0,  72,  78},	 //   3,  3,  0
			{  0,   0,  88,   0,  73,  84},	 //   3,  3,  1
			{  0,   0,  52,   0,  72,  92},	 //   3,  3,  2
			{  0,   0,  26,   0,  77,  94},	 //   3,  3,  3
			{  0,   0,  16,   0,  75,  90},	 //   3,  3,  4
			{  0,   0,   6,   0,  79,  85},	 //   3,  3,  5
			{  0,   0,   1,   0,  85,  86},	 //   3,  3,  6
			{  0,   0,   0,   0,  93,  96},	 //   3,  3,  7
		},
		{
			{  0,   0, 111,   0,  76,  32},	 //   3,  4,  0
			{  0,   0,  76,   0,  75,  39},	 //   3,  4,  1
			{  0,   0,  52,   0,  72,  44},	 //   3,  4,  2
			{  0,   0,  34,   0,  71,  50},	 //   3,  4,  3
			{  0,   0,  17,   0,  73,  50},	 //   3,  4,  4
			{  0,   0,   9,   0,  76,  49},	 //   3,  4,  5
			{  0,   0,   2,   0,  78,  48},	 //   3,  4,  6
			{  0,   0,   0,   0,  84,  61},	 //   3,  4,  7
		},
		{
			{  0,   0,  94,   0,  76,   6},	 //   3,  5,  0
			{  0,   0,  70,   0,  74,  13},	 //   3,  5,  1
			{  0,   0,  47,   0,  72,  14},	 //   3,  5,  2
			{  0,   0,  33,   0,  71,  16},	 //   3,  5,  3
			{  0,   0,  23,   0,  72,  19},	 //   3,  5,  4
			{  0,   0,   9,   0,  71,  20},	 //   3,  5,  5
			{  0,   0,   3,   0,  72,  19},	 //   3,  5,  6
			{  0,   0,   0,   0,  78,  25},	 //   3,  5,  7
		},
		{
			{  0,   0,  76,   0,  78,   0},	 //   3,  6,  0
			{  0,   0,  65,   0,  77,   3},	 //   3,  6,  1
			{  0,   0,  48,   0,  75,   4},	 //   3,  6,  2
			{  0,   0,  34,   0,  74,   4},	 //   3,  6,  3
			{  0,   0,  25,   0,  73,   6},	 //   3,  6,  4
			{  0,   0,  15,   0,  72,   5},	 //   3,  6,  5
			{  0,   0,   3,   0,  71,   3},	 //   3,  6,  6
			{  0,   0,   0,   0,  72,   4},	 //   3,  6,  7
		},
		{
			{  0,   0,  73,   0,  80,   0},	 //   3,  7,  0
			{  0,   0,  66,   0,  80,   0},	 //   3,  7,  1
			{  0,   0,  51,   0,  79,   0},	 //   3,  7,  2
			{  0,   0,  41,   0,  77,   0},	 //   3,  7,  3
			{  0,   0,  32,   0,  76,   0},	 //   3,  7,  4
			{  0,   0,  23,   0,  75,   0},	 //   3,  7,  5
			{  0,   0,  13,   0,  77,   0},	 //   3,  7,  6
			{  0,   0,   2,   0,  71,   0},	 //   3,  7,  7
		},
	},
	{
		{
			{  0, 173, 154,   0,  44,   0},	 //   4,  0,  0
			{  0, 187,  61,   0,  34,   0},	 //   4,  0,  1
			{  0, 150,  35,   0,  35,  19},	 //   4,  0,  2
			{  0, 107,  21,   0,  38,  83},	 //   4,  0,  3
			{  0,  95,  11,   0,  36,  99},	 //   4,  0,  4
			{  0,  29,   0,   0,  67, 189},	 //   4,  0,  5
			{  0,  14,   0,   0,  83, 200},	 //   4,  0,  6
			{  0,   6,   0,   0,  90, 203},	 //   4,  0,  7
		},
		{
			{  3,  70, 179,   0,  39, 115},	 //   4,  1,  0
			{  0,  47,  83,   0,  37, 163},	 //   4,  1,  1
			{  0,  29,  40,   0,  38, 186},	 //   4,  1,  2
			{  0,  21,  25,   0,  40, 192},	 //   4,  1,  3
			{  0,  19,  14,   0,  39, 193},	 //   4,  1,  4
			{  0,   8,   1,   0,  63, 199},	 //   4,  1,  5
			{  0,   0,   0,   0,  77, 201},	 //   4,  1,  6
			{  0,   0,   0,   0,  83, 193},	 //   4,  1,  7
		},
		{
			{  0,   2, 165,   0,  45, 145},	 //   4,  2,  0
			{  0,   0,  85,   0,  44, 160},	 //   4,  2,  1
			{  0,   0,  40,   0,  43, 161},	 //   4,  2,  2
			{  0,   0,  26,   0,  44, 158},	 //   4,  2,  3
			{  0,   0,  16,   0,  41, 155},	 //   4,  2,  4
			{  0,   0,   2,   0,  54, 151},	 //   4,  2,  5
			{  0,   0,   0,   0,  66, 149},	 //   4,  2,  6
			{  0,   0,   0,   0,  76, 158},	 //   4,  2,  7
		},
		{
			{  0,   0, 126,   0,  48,  81},	 //   4,  3,  0
			{  0,   0,  77,   0,  46,  88},	 //   4,  3,  1
			{  0,   0,  49,   0,  46,  95},	 //   4,  3,  2
			{  0,   0,  30,   0,  46,  97},	 //   4,  3,  3
			{  0,   0,  20,   0,  44, 100},	 //   4,  3,  4
			{  0,   0,   6,   0,  49,  94},	 //   4,  3,  5
			{  0,   0,   1,   0,  59,  94},	 //   4,  3,  6
			{  0,   0,   0,   0,  70, 105},	 //   4,  3,  7
		},
		{
			{  0,   0, 104,   0,  44,  45},	 //   4,  4,  0
			{  0,   0,  72,   0,  45,  50},	 //   4,  4,  1
			{  0,   0,  53,   0,  46,  56},	 //   4,  4,  2
			{  0,   0,  34,   0,  46,  57},	 //   4,  4,  3
			{  0,   0,  19,   0,  52,  58},	 //   4,  4,  4
			{  0,   0,  10,   0,  50,  57},	 //   4,  4,  5
			{  0,   0,   2,   0,  54,  59},	 //   4,  4,  6
			{  0,   0,   0,   0,  65,  66},	 //   4,  4,  7
		},
		{
			{  0,   0,  91,   0,  51,  14},	 //   4,  5,  0
			{  0,   0,  70,   0,  50,  15},	 //   4,  5,  1
			{  0,   0,  47,   0,  47,  20},	 //   4,  5,  2
			{  0,   0,  36,   0,  48,  23},	 //   4,  5,  3
			{  0,   0,  23,   0,  48,  26},	 //   4,  5,  4
			{  0,   0,  10,   0,  48,  26},	 //   4,  5,  5
			{  0,   0,   4,   0,  49,  26},	 //   4,  5,  6
			{  0,   0,   0,   0,  57,  33},	 //   4,  5,  7
		},
		{
			{  0,   0,  77,   0,  55,   0},	 //   4,  6,  0
			{  0,   0,  65,   0,  54,   4},	 //   4,  6,  1
			{  0,   0,  49,   0,  53,   6},	 //   4,  6,  2
			{  0,   0,  35,   0,  52,   6},	 //   4,  6,  3
			{  0,   0,  25,   0,  52,   8},	 //   4,  6,  4
			{  0,   0,  15,   0,  50,   7},	 //   4,  6,  5
			{  0,   0,   4,   0,  47,   8},	 //   4,  6,  6
			{  0,   0,   0,   0,  51,   9},	 //   4,  6,  7
		},
		{
			{  0,   0,  74,   0,  63,   0},	 //   4,  7,  0
			{  0,   0,  66,   0,  62,   0},	 //   4,  7,  1
			{  0,   0,  51,   0,  60,   0},	 //   4,  7,  2
			{  0,   0,  41,   0,  58,   0},	 //   4,  7,  3
			{  0,   0,  32,   0,  57,   0},	 //   4,  7,  4
			{  0,   0,  21,   0,  57,   0},	 //   4,  7,  5
			{  0,   0,  13,   0,  58,   0},	 //   4,  7,  6
			{  0,   0,   2,   0,  53,   0},	 //   4,  7,  7
		},
	},
	{
		{
			{  0, 164, 153,   0,  24,   0},	 //   5,  0,  0
			{  0, 155,  61,   0,  17,  17},	 //   5,  0,  1
			{  0, 110,  30,   0,  18,  76},	 //   5,  0,  2
			{  0,  79,  19,   0,  21, 125},	 //   5,  0,  3
			{  0,  61,  11,   0,  21, 148},	 //   5,  0,  4
			{  0,  53,   5,   0,  19, 161},	 //   5,  0,  5
			{  0,  17,   0,   0,  51, 198},	 //   5,  0,  6
			{  0,   5,   0,   0,  70, 203},	 //   5,  0,  7
		},
		{
			{  0,  54, 175,   0,  24, 146},	 //   5,  1,  0
			{  0,  34,  78,   0,  21, 181},	 //   5,  1,  1
			{  0,  24,  36,   0,  20, 191},	 //   5,  1,  2
			{  0,  16,  23,   0,  22, 197},	 //   5,  1,  3
			{  0,  13,  15,   0,  22, 198},	 //   5,  1,  4
			{  0,  11,   8,   0,  23, 198},	 //   5,  1,  5
			{  0,   2,   1,   0,  47, 201},	 //   5,  1,  6
			{  0,   0,   0,   0,  64, 193},	 //   5,  1,  7
		},
		{
			{  0,   2, 164,   0,  25, 160},	 //   5,  2,  0
			{  0,   1,  83,   0,  24, 170},	 //   5,  2,  1
			{  0,   0,  41,   0,  24, 167},	 //   5,  2,  2
			{  0,   0,  24,   0,  23, 158},	 //   5,  2,  3
			{  0,   0,  16,   0,  24, 153},	 //   5,  2,  4
			{  0,   0,   9,   0,  24, 153},	 //   5,  2,  5
			{  0,   0,   1,   0,  40, 153},	 //   5,  2,  6
			{  0,   0,   0,   0,  57, 157},	 //   5,  2,  7
		},
		{
			{  0,   0, 115,   0,  28,  89},	 //   5,  3,  0
			{  0,   0,  73,   0,  26,  95},	 //   5,  3,  1
			{  0,   0,  42,   0,  26,  98},	 //   5,  3,  2
			{  0,   0,  26,   0,  26,  99},	 //   5,  3,  3
			{  0,   0,  19,   0,  25,  98},	 //   5,  3,  4
			{  0,   0,  11,   0,  25,  98},	 //   5,  3,  5
			{  0,   0,   1,   0,  37, 101},	 //   5,  3,  6
			{  0,   0,   0,   0,  49, 113},	 //   5,  3,  7
		},
		{
			{  0,   0,  98,   0,  29,  51},	 //   5,  4,  0
			{  0,   0,  71,   0,  26,  54},	 //   5,  4,  1
			{  0,   0,  47,   0,  27,  57},	 //   5,  4,  2
			{  0,   0,  34,   0,  26,  62},	 //   5,  4,  3
			{  0,   0,  20,   0,  28,  63},	 //   5,  4,  4
			{  0,   0,  12,   0,  27,  65},	 //   5,  4,  5
			{  0,   0,   2,   0,  33,  66},	 //   5,  4,  6
			{  0,   0,   0,   0,  43,  75},	 //   5,  4,  7
		},
		{
			{  0,   0,  82,   0,  27,  23},	 //   5,  5,  0
			{  0,   0,  66,   0,  26,  26},	 //   5,  5,  1
			{  0,   0,  47,   0,  27,  29},	 //   5,  5,  2
			{  0,   0,  35,   0,  27,  30},	 //   5,  5,  3
			{  0,   0,  23,   0,  29,  31},	 //   5,  5,  4
			{  0,   0,  11,   0,  30,  32},	 //   5,  5,  5
			{  0,   0,   4,   0,  32,  31},	 //   5,  5,  6
			{  0,   0,   0,   0,  38,  39},	 //   5,  5,  7
		},
		{
			{  0,   0,  78,   0,  33,   0},	 //   5,  6,  0
			{  0,   0,  65,   0,  32,   6},	 //   5,  6,  1
			{  0,   0,  48,   0,  32,   8},	 //   5,  6,  2
			{  0,   0,  35,   0,  30,   9},	 //   5,  6,  3
			{  0,   0,  25,   0,  30,  10},	 //   5,  6,  4
			{  0,   0,  15,   0,  30,  11},	 //   5,  6,  5
			{  0,   0,   4,   0,  29,  12},	 //   5,  6,  6
			{  0,   0,   0,   0,  34,  13},	 //   5,  6,  7
		},
		{
			{  0,   0,  75,   0,  42,   0},	 //   5,  7,  0
			{  0,   0,  67,   0,  41,   0},	 //   5,  7,  1
			{  0,   0,  52,   0,  39,   0},	 //   5,  7,  2
			{  0,   0,  41,   0,  39,   0},	 //   5,  7,  3
			{  0,   0,  32,   0,  37,   0},	 //   5,  7,  4
			{  0,   0,  20,   0,  35,   0},	 //   5,  7,  5
			{  0,   0,  11,   0,  35,   0},	 //   5,  7,  6
			{  0,   0,   0,   0,  35,   0},	 //   5,  7,  7
		},
	},
	{
		{
			{  0, 121, 157,   0,   5,  58},	 //   6,  0,  0
			{  0, 103,  60,   0,   3,  85},	 //   6,  0,  1
			{  0,  85,  27,   0,   3, 113},	 //   6,  0,  2
			{  0,  56,  18,   0,   8, 155},	 //   6,  0,  3
			{  0,  37,  12,   0,  10, 181},	 //   6,  0,  4
			{  0,  29,   6,   0,   9, 187},	 //   6,  0,  5
			{  0,  24,   2,   0,  11, 193},	 //   6,  0,  6
			{  0,   8,   0,   0,  40, 202},	 //   6,  0,  7
		},
		{
			{  0,  42, 162,   0,   8, 161},	 //   6,  1,  0
			{  0,  25,  70,   0,   7, 192},	 //   6,  1,  1
			{  0,  18,  34,   0,   7, 196},	 //   6,  1,  2
			{  0,  12,  20,   0,   8, 198},	 //   6,  1,  3
			{  0,   7,  14,   0,  10, 199},	 //   6,  1,  4
			{  0,   6,   8,   0,   9, 198},	 //   6,  1,  5
			{  0,   3,   4,   0,  11, 200},	 //   6,  1,  6
			{  0,   0,   0,   0,  37, 199},	 //   6,  1,  7
		},
		{
			{  0,   0, 139,   0,  11, 165},	 //   6,  2,  0
			{  0,   1,  79,   0,  10, 166},	 //   6,  2,  1
			{  0,   0,  42,   0,  10, 170},	 //   6,  2,  2
			{  0,   0,  24,   0,  10, 165},	 //   6,  2,  3
			{  0,   0,  15,   0,   9, 155},	 //   6,  2,  4
			{  0,   0,  10,   0,   9, 154},	 //   6,  2,  5
			{  0,   0,   5,   0,  10, 154},	 //   6,  2,  6
			{  0,   0,   0,   0,  34, 162},	 //   6,  2,  7
		},
		{
			{  0,   0, 115,   0,  13,  98},	 //   6,  3,  0
			{  0,   0,  72,   0,  12,  99},	 //   6,  3,  1
			{  0,   0,  42,   0,  12, 100},	 //   6,  3,  2
			{  0,   0,  26,   0,  11, 103},	 //   6,  3,  3
			{  0,   0,  16,   0,  12,  99},	 //   6,  3,  4
			{  0,   0,  11,   0,  11,  99},	 //   6,  3,  5
			{  0,   0,   6,   0,  10, 101},	 //   6,  3,  6
			{  0,   0,   0,   0,  30, 118},	 //   6,  3,  7
		},
		{
			{  0,   0,  94,   0,  14,  55},	 //   6,  4,  0
			{  0,   0,  68,   0,  13,  58},	 //   6,  4,  1
			{  0,   0,  46,   0,  11,  61},	 //   6,  4,  2
			{  0,   0,  29,   0,  12,  63},	 //   6,  4,  3
			{  0,   0,  18,   0,  13,  66},	 //   6,  4,  4
			{  0,   0,  11,   0,  13,  66},	 //   6,  4,  5
			{  0,   0,   6,   0,  12,  69},	 //   6,  4,  6
			{  0,   0,   0,   0,  27,  80},	 //   6,  4,  7
		},
		{
			{  0,   0,  77,   0,  14,  26},	 //   6,  5,  0
			{  0,   0,  65,   0,  13,  27},	 //   6,  5,  1
			{  0,   0,  46,   0,  12,  31},	 //   6,  5,  2
			{  0,   0,  34,   0,  13,  33},	 //   6,  5,  3
			{  0,   0,  23,   0,  13,  36},	 //   6,  5,  4
			{  0,   0,  11,   0,  13,  37},	 //   6,  5,  5
			{  0,   0,   8,   0,  13,  36},	 //   6,  5,  6
			{  0,   0,   0,   0,  24,  45},	 //   6,  5,  7
		},
		{
			{  0,   0,  77,   0,  12,   8},	 //   6,  6,  0
			{  0,   0,  65,   0,  12,   9},	 //   6,  6,  1
			{  0,   0,  47,   0,  12,  11},	 //   6,  6,  2
			{  0,   0,  34,   0,  13,  13},	 //   6,  6,  3
			{  0,   0,  25,   0,  14,  13},	 //   6,  6,  4
			{  0,   0,  14,   0,  15,  14},	 //   6,  6,  5
			{  0,   0,   6,   0,  15,  14},	 //   6,  6,  6
			{  0,   0,   0,   0,  22,  20},	 //   6,  6,  7
		},
		{
			{  0,   0,  75,   0,  25,   0},	 //   6,  7,  0
			{  0,   0,  67,   0,  25,   0},	 //   6,  7,  1
			{  0,   0,  52,   0,  24,   0},	 //   6,  7,  2
			{  0,   0,  39,   0,  23,   0},	 //   6,  7,  3
			{  0,   0,  30,   0,  21,   0},	 //   6,  7,  4
			{  0,   0,  19,   0,  21,   0},	 //   6,  7,  5
			{  0,   0,   9,   0,  19,   0},	 //   6,  7,  6
			{  0,   0,   0,   0,  19,   0},	 //   6,  7,  7
		},
	},
	{
		{
			{  0,  76,  98,   0,   0, 123},	 //   7,  0,  0
			{  0,  55,  54,   0,   0, 157},	 //   7,  0,  1
			{  0,  47,  28,   0,   0, 168},	 //   7,  0,  2
			{  0,  33,  18,   0,   0, 186},	 //   7,  0,  3
			{  0,  26,  13,   0,   0, 190},	 //   7,  0,  4
			{  0,  19,  10,   0,   0, 196},	 //   7,  0,  5
			{  0,  14,   2,   0,   0, 199},	 //   7,  0,  6
			{  0,  14,   0,   0,   0, 198},	 //   7,  0,  7
		},
		{
			{  0,  27, 106,   0,   0, 177},	 //   7,  1,  0
			{  0,  17,  62,   0,   0, 197},	 //   7,  1,  1
			{  0,  13,  32,   0,   0, 199},	 //   7,  1,  2
			{  0,   9,  18,   0,   0, 199},	 //   7,  1,  3
			{  0,   4,  15,   0,   0, 200},	 //   7,  1,  4
			{  0,   1,  10,   0,   0, 199},	 //   7,  1,  5
			{  0,   0,   6,   0,   0, 197},	 //   7,  1,  6
			{  0,   0,   0,   0,   0, 200},	 //   7,  1,  7
		},
		{
			{  0,   0, 120,   0,   0, 162},	 //   7,  2,  0
			{  0,   0,  68,   0,   0, 167},	 //   7,  2,  1
			{  0,   0,  39,   0,   0, 167},	 //   7,  2,  2
			{  0,   0,  23,   0,   0, 164},	 //   7,  2,  3
			{  0,   0,  14,   0,   0, 158},	 //   7,  2,  4
			{  0,   0,  11,   0,   0, 154},	 //   7,  2,  5
			{  0,   0,   7,   0,   0, 151},	 //   7,  2,  6
			{  0,   0,   0,   0,   0, 160},	 //   7,  2,  7
		},
		{
			{  0,   0, 105,   0,   0, 100},	 //   7,  3,  0
			{  0,   0,  72,   0,   0, 104},	 //   7,  3,  1
			{  0,   0,  44,   0,   0, 108},	 //   7,  3,  2
			{  0,   0,  26,   0,   0, 106},	 //   7,  3,  3
			{  0,   0,  15,   0,   0, 105},	 //   7,  3,  4
			{  0,   0,  11,   0,   0, 100},	 //   7,  3,  5
			{  0,   0,   8,   0,   0,  99},	 //   7,  3,  6
			{  0,   0,   0,   0,   0, 118},	 //   7,  3,  7
		},
		{
			{  0,   0,  94,   0,   0,  63},	 //   7,  4,  0
			{  0,   0,  68,   0,   0,  62},	 //   7,  4,  1
			{  0,   0,  45,   0,   0,  66},	 //   7,  4,  2
			{  0,   0,  29,   0,   0,  69},	 //   7,  4,  3
			{  0,   0,  16,   0,   0,  70},	 //   7,  4,  4
			{  0,   0,  10,   0,   0,  70},	 //   7,  4,  5
			{  0,   0,   7,   0,   0,  68},	 //   7,  4,  6
			{  0,   0,   0,   0,   0,  85},	 //   7,  4,  7
		},
		{
			{  0,   0,  78,   0,   0,  29},	 //   7,  5,  0
			{  0,   0,  65,   0,   0,  31},	 //   7,  5,  1
			{  0,   0,  45,   0,   0,  34},	 //   7,  5,  2
			{  0,   0,  33,   0,   0,  37},	 //   7,  5,  3
			{  0,   0,  23,   0,   0,  40},	 //   7,  5,  4
			{  0,   0,  11,   0,   0,  42},	 //   7,  5,  5
			{  0,   0,   7,   0,   0,  40},	 //   7,  5,  6
			{  0,   0,   0,   0,   0,  49},	 //   7,  5,  7
		},
		{
			{  0,   0,  75,   0,   0,   9},	 //   7,  6,  0
			{  0,   0,  65,   0,   0,  11},	 //   7,  6,  1
			{  0,   0,  47,   0,   0,  15},	 //   7,  6,  2
			{  0,   0,  34,   0,   0,  14},	 //   7,  6,  3
			{  0,   0,  26,   0,   0,  17},	 //   7,  6,  4
			{  0,   0,  15,   0,   0,  18},	 //   7,  6,  5
			{  0,   0,   4,   0,   0,  21},	 //   7,  6,  6
			{  0,   0,   0,   0,   0,  20},	 //   7,  6,  7
		},
		{
			{  0,   0,  78,   0,   0,   0},	 //   7,  7,  0
			{  0,   0,  68,   0,   0,   0},	 //   7,  7,  1
			{  0,   0,  51,   0,   0,   0},	 //   7,  7,  2
			{  0,   0,  39,   0,   0,   0},	 //   7,  7,  3
			{  0,   0,  27,   0,   0,   0},	 //   7,  7,  4
			{  0,   0,  17,   0,   0,   0},	 //   7,  7,  5
			{  0,   0,   8,   0,   0,   0},	 //   7,  7,  6
			{  0,   0,   0,   0,   0,   0},	 //   7,  7,  7
		},
	},
};
