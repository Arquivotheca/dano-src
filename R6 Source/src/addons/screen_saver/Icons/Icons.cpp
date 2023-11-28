
#include <Bitmap.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Screen.h>
#include <StringView.h>

#include <stdlib.h>
#include <time.h>

#include "Icons.h"

extern "C" _EXPORT BScreenSaver *
instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Icons(message, image);
}

Icons::Icons(BMessage *message, image_id image)
	:	BScreenSaver(message, image)
{
}


void
Icons::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35),
		B_EMPTY_STRING, "Icons: a desktop munger test"));
}

const int32 kMaxBlitters = 10;
IconBlitState *blitters[kMaxBlitters];

status_t
Icons::StartSaver(BView *view, bool)
{
	srand(time(0));
	for (int32 index = 0; index < kMaxBlitters; index++)
		blitters[index] = 0;

	SetTickSize(5000);
	return B_OK;
}



MimeTypeIterator::MimeTypeIterator()
	:	typeCount(0),
		typeIndex(0),
		firstTime(0)
{
	for (int32 index = 0; index < 10; index++)
		cachedTypes[index] = 0;
}


MimeTypeIterator::~MimeTypeIterator()
{
	for (int32 index = 0; index < 10; index++)
		delete cachedTypes[index];
}


const char *kTypesDescriptor = "types";
const char *kSuperTypesDescriptor = "super_types";

void 
MimeTypeIterator::NextBitmap(BBitmap *bitmap, int32 rand)
{
	uint32 type = B_STRING_TYPE;
	if (firstTime) {
		BMimeType::GetInstalledSupertypes(&supertypes);
		supertypes.GetInfo(kSuperTypesDescriptor, &type, &supertypeCount);
		
		supertypeIndex = supertypeCount;
		if (supertypeCount > 10)
			supertypeCount = 10;
	}

	typeIndex += rand;

	for (;;) {
		if (typeIndex >= typeCount) {
	
			if (supertypeIndex >= supertypeCount)
				supertypeIndex = 0;
	
			const char *supertypeName;
			supertypes.FindString(kSuperTypesDescriptor, supertypeIndex, 
				&supertypeName);
			
			
			if (!cachedTypes[supertypeIndex]) {
				cachedTypes[supertypeIndex] = new BMessage;
				BMimeType::GetInstalledTypes(supertypeName, cachedTypes[supertypeIndex]);
			}
			types = cachedTypes[supertypeIndex];
	
			supertypeIndex++;
			typeIndex = 0;
	
			types->GetInfo(kTypesDescriptor, &type, &typeCount);
		}
	
		const char *mimeTypeName;
		types->FindString(kTypesDescriptor, typeIndex++, &mimeTypeName);
		BMimeType mime(mimeTypeName);
		
		if (mime.GetIcon(bitmap, B_LARGE_ICON) == B_OK)
			return;
	}
}


IconBlitState *blitState = 0;

const char *mimeTypes[] = {
	"application/x-person",
	"application/x-vnd.Be-query",
	"application/x-vnd.Be-root",
	"application/x-vnd.Be-volume",
	"application/x-vnd.Be-bookmark",
	"application/x-vnd.Be-directory",
	"application/x-vnd.Be-symlink"
};

MimeTypeIterator iconRandomizer;

void
Icons::Draw(BView *view, int32 frame)
{
	bool preview = view->Bounds().Width() < 640;

	if (frame == 0)
		view->FillRect(view->Bounds(), B_SOLID_LOW);


	for (int32 count = 5; count; count--) {
		int32 blitterIndex = rand() % kMaxBlitters;
		if (!blitters[blitterIndex]) {
			int32 count = sizeof(mimeTypes) / sizeof(const char *);
			const char *type = mimeTypes[rand() % count];
			
			BPoint point;
			int32 scale;
			for (;;) {
				point.x = (rand() % view->Bounds().IntegerHeight());
				point.y = (rand() % view->Bounds().IntegerWidth());
				scale = preview ? (rand() % 3) : (rand() % 8 + 2);
	
				BRect frame(IconBlitState::FrameForValue(point, scale));
				
				bool intersects = false;
				for (int32 index = 0; index < kMaxBlitters; index++) 
					if (blitters[index] && frame.Intersects(blitters[index]->Frame())) {
						// don't overlay existing icons
						intersects = true;
						break;
					}
	
				if (!intersects)
					break;
			}
			
			BMessage supertypes;
			BMimeType::GetInstalledSupertypes(&supertypes);
	
			BMimeType mime(type);
			BBitmap icon(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
			iconRandomizer.NextBitmap(&icon, rand() % 10);
	
			blitters[blitterIndex] = new IconBlitState(&icon, point, scale); 
		}
		if (!blitters[blitterIndex]->BlitOne(view)) {
			delete blitters[blitterIndex];
			blitters[blitterIndex] = 0;
		}
	}
	
}


IconBlitState::IconBlitState(const BBitmap *icon, BPoint where, int32 scale)
	:	where(where),
		original(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT),
		level(0.9),
		rampLevel(1.015),
//		rampLevel(0.02),
		scale(scale),
		rampUp(false)
{
	original.SetBits(icon->Bits(), original.BitsLength(), 0, B_COLOR_8_BIT);
}

const rgb_color kBlack = {0, 0, 0, 255};

inline rgb_color
ShiftDown(rgb_color color, float factor)
{
	if (factor <= 0)
		return kBlack;

	if (factor > 1)
		return color;

	color.red = (uint8)(color.red * factor + 0.5);
	color.green = (uint8)(color.green * factor + 0.5);
	color.blue = (uint8)(color.blue * factor + 0.5);
	return color;
}

BRect 
IconBlitState::Frame() const
{
	return FrameForValue(where, scale);
}

BRect 
IconBlitState::FrameForValue(BPoint where, int32 scale)
{
	BRect destRect(0, 0, (scale * B_LARGE_ICON) - 1, (scale * B_LARGE_ICON) - 1);
	destRect.OffsetTo(where);
	return destRect;
}

bool
IconBlitState::BlitOne(BView *view)
{
	BBitmap shiftedBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_RGBA32);
	uchar *bits = (uchar *)shiftedBitmap.Bits();
	uchar *originalBits = (uchar *)original.Bits();
	int32 bitsLength = original.BitsLength();


	BScreen screen(B_MAIN_SCREEN_ID);
	rgb_color color;

	for (int32 index = 0; index < bitsLength; index++) {
		int32 original = originalBits[index];
		if (original == B_TRANSPARENT_8_BIT)
			color = B_TRANSPARENT_32_BIT;
		else {
			color = screen.ColorForIndex(originalBits[index]);
			color = ShiftDown(color, 1 - level);
		}
		*bits++ = color.blue;
		*bits++ = color.green;
		*bits++ = color.red;
		*bits++ = color.alpha;
	}
	
	BRect destRect(Frame());

	view->SetDrawingMode(B_OP_OVER);
	view->DrawBitmap(&shiftedBitmap, destRect);
	
	if (level < 0.1)
		rampUp = true;

	if (rampUp)
//		level += rampLevel;
		level *= rampLevel;
	else
//		level -= rampLevel;
		level /= rampLevel;


	return level < 1;
}

