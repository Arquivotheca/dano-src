
#include <ScreenSaver.h>
#include <Bitmap.h>

#include <map>

class Icons : public BScreenSaver {
public:
	
	Icons(BMessage *, image_id );
	
	virtual void StartConfig(BView *);
	virtual status_t StartSaver(BView *, bool);
	virtual void Draw(BView *, int32);
};

class IconBlitState {
public:
	IconBlitState(const BBitmap *icon, BPoint where, int32 scale);
	bool BlitOne(BView *);

	BRect Frame() const;
	static BRect FrameForValue(BPoint, int32);

private:
	BPoint where;
	BBitmap original;
	float level;
	float rampLevel;
	int32 scale;
	bool rampUp;
};

class MimeTypeIterator {
public:
	MimeTypeIterator();
	virtual ~MimeTypeIterator();
	void NextBitmap(BBitmap *bitmap, int32 rand);

private:
	int32 supertypeCount;
	int32 supertypeIndex;
	BMessage supertypes;
	
	int32 typeCount;
	int32 typeIndex;
	BMessage *types;

	bool firstTime;
	
	BMessage *cachedTypes[10];
};
