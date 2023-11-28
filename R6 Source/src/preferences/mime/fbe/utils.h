#include <stdio.h>
#include <stdlib.h>

#include <GraphicsDefs.h>
#include <Rect.h>
#include <Screen.h>
#include <View.h>

#include "FatBitDefs.h"

#include <interface_misc.h>

color_map* ColorMap();
uint8 IndexForColor(rgb_color c);
uint8 IndexForColor(uint8 r, uint8 g, uint8 b);
rgb_color ColorForIndex(uint8 i);

BRect MakeValidRect(BRect r);
void ConstrainRect(bool constrain, int32 *sX, int32 *sY, int32 *eX, int32 *eY);
BRect IntersectRects(BRect r1, BRect r2);

void AddBevel(BView* v, BRect bounds, bool outset);
void AddRaisedBevel(BView* v, BRect bounds, bool outset);

void DrawFancyBorder(BView*);

void PrintRGB(rgb_color c);
bool CompareRGB( rgb_color c1, rgb_color c2);
