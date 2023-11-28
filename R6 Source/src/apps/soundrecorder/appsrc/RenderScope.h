#ifndef RENDER_SCOPE_H
#define RENDER_SCOPE_H

#include <GraphicsDefs.h>

class BBitmap;

void render_scope( 	BBitmap *bitmap, 
					BRect destRect, 
					rgb_color color, 
					float *elements,
					int32 elementCount );

inline uint8 inverse_cm_lookup( const color_map *cmap, const rgb_color &color )
{
	return cmap->index_map[(int32(color.red >> 3) << 10) | (int32(color.green >> 3) << 5) | int32(color.blue >> 3)];
}				

inline rgb_color cm_lookup( const color_map *cmap, uint8 index )
{
	return cmap->color_list[index];
}

#endif
