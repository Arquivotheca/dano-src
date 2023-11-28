
#ifndef LNG_TIFFREADER
#define	LNG_TIFFREADER

#include <InterfaceDefs.h>
#include "tiffio.h"
#include "tiff.h"

class TIFFReader
{
public:
	TIFFReader(const char *name);
	~TIFFReader();
		
	bool	CanReadImage();

	void	GetBkColor(rgb_color *color);
	
	int		GetImageSize(int *w, int *h);
	int		GetImageResolution(float *x, float *y);
	bool	GetImageData(rgb_color *data);

	bool	ReadBlock(rgb_color *data, int dw, int dh, int l, int t, int r, int b);

	
protected:
	TIFF 	*tiff;
} ;


#endif	//LNG_TIFFREADER
