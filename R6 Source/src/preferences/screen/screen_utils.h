#include <GraphicsDefs.h>

enum resolution {
	k640x480Resolution,
	k800x600Resolution,
	k1024x768Resolution,
	k1152x900Resolution,
	k1280x1024Resolution,
	k1600x1200Resolution,
	kNoResolution
};

//	set/get refresh rate for current workspace
void SetRefreshRate(float rate);
float RefreshRate();

//	get rate limits for monitor
void RateLimits(float *min, float *max);

// set/get desktop color for current workspace
void SetDesktopColor(rgb_color c);
rgb_color DesktopColor(void);

// set/get resolution for current workspace
void SetResolution(color_space c, float w, float h);
resolution Resolution();

uint32 MonitorColorSpace();

// returns screen dimensions for current workspace
float ScreenWidth();
float ScreenHeight();

// conversion to/from resolution constant to actual dimensions
resolution ResolutionFor(float w, float h);
void DimensionsFor(resolution r, float *w, float *h);

//	set/get for colorspace for current workspace
void SetBitsPerPixel(color_space bpp, float w, float h);
color_space BitsPerPixel();

uint32 ColorCount(color_space bpp);

//	conversion for components to color/res constant
uint32 PartsToScreenSpace(color_space bpp,float w,float h);

// 	monitor config
void SetCRTPosition(uchar h_pos, uchar v_pos, uchar h_size, uchar v_size);
