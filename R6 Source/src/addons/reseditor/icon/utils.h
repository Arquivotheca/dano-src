#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

#include <ColorControl.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <Screen.h>
#include <View.h>

#include <experimental/ColorTools.h>

#include "FatBitDefs.h"

class BMenuField;
struct translator_info;
struct TranslatorBitmap;
class BBitmap;
class BMessage;

enum {
	B_CURSOR_TYPE			= 'CURS',
	B_BITMAP_TYPE			= 'BBMP'
};

color_map* ColorMap();
uint8 IndexForColor(rgb_color c);
uint8 IndexForColor(uint8 r, uint8 g, uint8 b);
rgb_color ColorForIndex(uint8 i);

BRect MakeValidRect(BRect r);
void ConstrainRect(bool constrain, int32 *sX, int32 *sY, int32 *eX, int32 *eY);
BRect IntersectRects(BRect r1, BRect r2);
BRect ContainingRect(BPoint p1, BPoint p2);
BRect ExtendRectPoint(BRect r, BPoint p);
BRect ExtendRects(BRect r1, BRect r2);

void AddBevel(BView* v, BRect bounds, bool outset);
void AddRaisedBevel(BView* v, BRect bounds, bool outset);

void DrawFancyBorder(BView*);

float GetMenuFieldSize(BMenuField* field, float* width, float* height);

status_t BitmapFromCursor(const uint8* data, size_t size,
						  translator_info* out_info = 0,
						  TranslatorBitmap* out_header = 0,
						  BMessage* out_io_extension = 0,
						  BBitmap** out_bitmap = 0);
uint8* CursorFromBitmap(const BBitmap* bm,
						const BMessage* io_extension,
						size_t* out_size);

void PrintRGB(rgb_color c);
bool CompareRGB( rgb_color c1, rgb_color c2);

class TColorControl : public BColorControl {
public:
	TColorControl(	BPoint start,
					color_control_layout layout,
					float cell_size,
					const char *name,
					BMessage *message = NULL,
					bool use_offscreen = false);
	~TColorControl();
	
	void SetModificationMessage(BMessage* msg);
	BMessage* ModificationMessage() const;
	
	void MouseDown(BPoint where);
	void MouseMoved(BPoint where, uint32 code, const BMessage *);
	
private:
	BMessage* fModificationMessage;
	rgb_color fLastColor;
};

#endif
