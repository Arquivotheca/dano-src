#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <CursorManager.h>
#include <Debug.h>
#include <File.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

#define CURSOR_MANAGER_DEBUG 0
#define MAX_BUILTIN_CURSORS 100

static vint32 queueCounter(1);
static vint32 cursorCounter(MAX_BUILTIN_CURSORS); // leave some entries for built-in cursor tokens

static const char* CURSOR_DIR = "/boot/custom/resources/en/Cursors";

static void* Load32x32PNGCursor(const char *filename, uint16 hotspotX, uint16 hotspotY,
								rgb_color high, rgb_color low);

// queue_node does not own the data it points to, so it does not delete it
struct BCursorManager::queue_node {
				queue_node(cursor_data *d, uint8 p) {
					data = d;
					cursorData = NULL;
					priority = p;
					token = atomic_add(&queueCounter, 1);
					next = NULL;
					nonremovable = false;
				}

	queue_token		token;
	cursor_data*	data;		// set to NULL for built-in cursors
	void*			cursorData; // points to built-in cursor data
	queue_node*		next;
	uint8			priority;
	bool			nonremovable;	// true for the default cursor
};


//---------------------------------------------------------------------------

BCursorAnimator::BCursorAnimator(BCursorManager *manager)
	: GHandler("BCursorAnimator"),
	  fManager(manager)
{
}

BCursorAnimator::~BCursorAnimator()
{
}

status_t
BCursorAnimator::HandleMessage(BMessage *message)
{
	switch (message->what) {
	  case 'ANIM':
	  	{
			// dequeue other 'ANIM' messages so that we can't get into
			// a situation where messages pile up, causing the cursor to
			// animate too quickly.  This could happen when there are
			// multiple animated cursors in the cursor queue that have
			// each been shown at least once, in which case each of them
			// would have posted an 'ANIM' message.
			BMessage *animMsg;
			while ((animMsg = DequeueMessage('ANIM')) != NULL) {
				delete animMsg;
			}
			fManager->SetNextCursor();
		}
	  	break;
	  	
	  default:
		// do nothing
		break;
	}
	return GHandler::HandleMessage(message);
}
	
//---------------------------------------------------------------------------

// the global BCursorManager
BCursorManager cursorManager;

#if CURSOR_MANAGER_DEBUG
// Prints out the cursor queue -- useful for debugging
void PrintQueue(BCursorManager::queue_node *head) {
	fprintf(stderr, "head: ");
	while (head != NULL) {
		const char *name = (head->data == NULL) ? "BUILT-IN" : head->data->name.String();
		fprintf(stderr, "(%s, %ld, p=%d) -> ", name, head->token, head->priority);
		head = head->next;
	}
	fprintf(stderr, "(null)\n");
}
#endif

BCursorManager::BCursorManager()
	: fCursors(NULL),
	  fQueue(NULL),
	  fLastCursorToken(0),
	  fNextAnimFrame(0),
	  fInitialized(false)
{
}

BCursorManager::~BCursorManager()
{
	BAutolock al(fLock);

	cursor_data* tmp = NULL;
	while (fCursors != NULL) {
		tmp = fCursors->next;
		delete fCursors;
		fCursors = tmp;
	}
	
	queue_node* tmp2 = NULL;
	while (fQueue != NULL) {
		tmp2 = fQueue->next;
		delete fQueue;
		fQueue = tmp2;
	}
}

void
BCursorManager::Initialize()
{
	BAutolock al(fLock);

	if (!fInitialized) {
		// create animation handler
		fAnimator = new BCursorAnimator(this);
	
		// load normal cursor
		cursor_data *data = new cursor_data();
		data->name = "cursor";	// XXX: hardcoded default cursor name
		data->hotspotX = 1;
		data->hotspotY = 6;
		if (LoadCursor(data) == B_OK) {
			data->next = NULL;
			fCursors = data;
			fQueue = new queue_node(data, 0);
		} else {
			// default to old hand cursor if we don't find the PNG cursor
#if CURSOR_MANAGER_DEBUG
			fprintf(stderr, "BCursorManager::Initialize: failed to load default cursor!\n");
#endif
			delete data;
			fQueue = new queue_node(NULL, 0);
			fQueue->cursorData = (void *)B_HAND_CURSOR;
		}
		fQueue->nonremovable = true; // prevent the default cursor from being removed
		
		SetNextCursor();
		fInitialized = true;
	}
}

status_t
BCursorManager::GetCursorToken(cursor_type typeCode, cursor_token *outToken)
{
	status_t err = B_OK;	
	switch (typeCode) {
	  case B_HAND_CURSOR_TYPE:
		*outToken = (cursor_token)B_HAND_CURSOR_TYPE;
		break;

	  case B_I_BEAM_CURSOR_TYPE:
		*outToken = (cursor_token)B_I_BEAM_CURSOR_TYPE;
		break;

	  default:
		err = B_ERROR;
	}
	return err;
}

status_t		
BCursorManager::GetCursorToken(cursor_data* data, cursor_token *outToken)
{
	BAutolock al(fLock);
	if (!fInitialized) Initialize();
	
	cursor_data *cursor_ptr = FindCursor(data);
	if (cursor_ptr != NULL) {
		*outToken = cursor_ptr->token;
		return B_OK;
	}
	
	status_t err = B_ERROR;
	// allocate new cursor and copy data into it
	cursor_ptr = new cursor_data();
	*cursor_ptr = *data;
	
	if ((err = LoadCursor(cursor_ptr)) == B_OK) {
		if (fCursors) {
			// add new cursor to head of the list
			cursor_ptr->next = fCursors;
			fCursors = cursor_ptr;
		} else {
			// no cursors yet loaded, so the new one is the head of the list
			cursor_ptr->next = NULL;
			fCursors = cursor_ptr;
		}
		*outToken = cursor_ptr->token;
		err = B_OK;
	} else {
		delete cursor_ptr;
	}
	return err;
}
		

status_t
BCursorManager::SetCursor(cursor_token cursor, uint8 priority, queue_token *outToken)
{
	// The highest priority is at the head of the queue which means that
	// it is currently showing.  The most recently added cursor of a certain
	// priority takes precedence over previously added cursors of the same priority
	BAutolock al(fLock);

	if (!fInitialized) Initialize();
#if CURSOR_MANAGER_DEBUG
		fprintf(stderr, "SetCursor(%ld, p=%d) entered, current queue: ", cursor, priority);
		PrintQueue(fQueue);
#endif
	queue_node *nuNode = NULL;
	if (cursor < MAX_BUILTIN_CURSORS) {
		// create a queue node for a built-in cursor
		void *cursorData = NULL;
		switch (cursor) {
		  case B_HAND_CURSOR_TYPE:
			cursorData = (void*)B_HAND_CURSOR;
			break;
	
		  case B_I_BEAM_CURSOR_TYPE:
			cursorData = (void*)B_I_BEAM_CURSOR;
			break;
	
		  default:
			break;
		}
		
		if (cursorData != NULL) {
			nuNode = new queue_node(NULL, priority);
			nuNode->cursorData = cursorData;
		}
	} else {
		// create a queue node for a custom cursor
		cursor_data* cursor_ptr = FindCursor(cursor);
		if (cursor_ptr) {
			nuNode = new queue_node(cursor_ptr, priority);
		}
	}

	if (nuNode == NULL) {
		*outToken = 0;
		return B_BAD_VALUE;
	}

	if (fQueue) {
		// add this node at the appropriate position in the queue
		queue_node *tmp = fQueue;
		queue_node *prev = NULL;
		while (tmp != NULL) {
			if (nuNode->priority >= tmp->priority) {
				// insert nuNode before tmp
				nuNode->next = tmp;
				if (prev != NULL) {
					prev->next = nuNode;
				} else {
					fQueue = nuNode;
					SetNextCursor();	
				}
				break;
			}
			
			prev = tmp;
			tmp = tmp->next;
		}
		
		if (tmp == NULL) {
			// add at end of list
			prev->next = nuNode;
		}
	} else {
		fQueue = nuNode;
		SetNextCursor();	
	}

#if CURSOR_MANAGER_DEBUG
	fprintf(stderr, "SetCursor(%ld, p=%d) exiting, current queue: ", cursor, priority);
	PrintQueue(fQueue);
#endif

	*outToken = nuNode->token;
	return B_OK;
}


bool
BCursorManager::RemoveCursor(queue_token token)
{
	// returns true if the queue_token is found and removed
	bool ret = false;
	BAutolock al(fLock);

#if CURSOR_MANAGER_DEBUG
	fprintf(stderr, "RemoveCursor(%ld) entering, current queue: ", token);
	PrintQueue(fQueue);
#endif

	queue_node* ptr = fQueue;
	queue_node* prev = NULL;
	while (ptr != NULL) {
		if (ptr->token == token) {
			if (ptr->nonremovable) {
				// don't remove a nonremovable queue entry
				fprintf(stderr, "BCursorManager: attempted to remove a nonremovable cursor!\n");
				break;	
			}
			
			if (prev != NULL) {
				prev->next = ptr->next;
			} else {
				fQueue = ptr->next;
				SetNextCursor();
			}
			delete ptr;
			ret = true;
			break;
		}
		
		prev = ptr;
		ptr = ptr->next;
	}

#if CURSOR_MANAGER_DEBUG
	fprintf(stderr, "RemoveCursor(%ld) exiting, current queue: ", token);
	PrintQueue(fQueue);
#endif

	return ret;
}

void
BCursorManager::SetNextCursor()
{
	// Determines which cursor to show on screen next and then sets the cursor.
	// This method is called either because the queue has changed or because the
	// next animation frame of the current cursor should be displayed.

	BAutolock al(fLock);
	
	if (fQueue == NULL) {
		return;
	}

	void *nextCursor = NULL;
	
	if (fLastCursorToken == fQueue->token) {
		// already showing this cursor, so show next animation frame
		if (fQueue->data != NULL) {
			uint32 maxFrames = MAX_CURSOR_ANIM_FRAMES;
			fNextAnimFrame = fNextAnimFrame % maxFrames;
			nextCursor = fQueue->data->rawData[fNextAnimFrame];
			fNextAnimFrame++;
			if (nextCursor == NULL) {
				fNextAnimFrame = 1;
				nextCursor = fQueue->data->rawData[0];		
			}
		}
	} else {
		// head of queue has changed, so show new cursor
		fLastCursorToken = fQueue->token;
		fNextAnimFrame = 1;
		nextCursor = (fQueue->data == NULL) ? fQueue->cursorData : fQueue->data->rawData[0];
	}

	if (nextCursor != NULL) {
		be_app->SetCursor(nextCursor);
	}

	// post animation message if this cursor has more than one frame
	if ((fQueue->data != NULL) && fQueue->data->rawData[1] != NULL) {
		BMessage msg('ANIM');
		bigtime_t delay = fQueue->data->animationDelay;
		if (delay < 50000) delay = 50000; // don't allow animations faster than 20 fps
		fAnimator->PostDelayedMessage(msg, delay);
	}
}

BCursorManager::cursor_data*
BCursorManager::FindCursor(cursor_data *data)
{
	BAutolock al(fLock);
	
	if (data != NULL) {
		cursor_data *ptr = fCursors;
		while (ptr != NULL) {
			if (*data == *ptr) {
				return ptr;
			}
			ptr = ptr->next;
		}
	}	
	return NULL;
}

BCursorManager::cursor_data*
BCursorManager::FindCursor(cursor_token token)
{
	BAutolock al(fLock);
	
	cursor_data *ptr = fCursors;
	while (ptr != NULL) {
		if (token == ptr->token) {
			return ptr;
		}
		ptr = ptr->next;
	}

	return NULL;
}

status_t
BCursorManager::LoadCursor(cursor_data* data)
{
	if (data == NULL) {
		return B_ERROR;
	}

	bool absolutePath = (data->name[0] == '/');
	char pathname[B_PATH_NAME_LENGTH];

	if (absolutePath) {
		strncpy(pathname, data->name.String(), sizeof(pathname) - 1);
	} else {
		// build a path relative to the cursor directory
		snprintf(pathname, sizeof(pathname) - 1, "%s/%s.png", CURSOR_DIR, data->name.String());
	}
	
	void *rawData = Load32x32PNGCursor(	pathname, data->hotspotX, data->hotspotY,
										data->highColor, data->lowColor);
	if ((!absolutePath) && (rawData == NULL)) {
		// couldn't find name.png, so let's try name01.png and look for an animation
		for (uint32 i = 0; i < MAX_CURSOR_ANIM_FRAMES; i++) {
			snprintf(	pathname, sizeof(pathname) - 1, "%s/%s%02ld.png",
						CURSOR_DIR, data->name.String(), i+1);
			rawData = Load32x32PNGCursor(	pathname, data->hotspotX, data->hotspotY,
											data->highColor, data->lowColor);
			if (rawData) {
				data->rawData[i] = rawData;
			} else {
				break;
			}
		}
	} else {
		data->rawData[0] = rawData;
	}
	
	if (data->rawData[0] != NULL) {
		// assign a token
		data->token = atomic_add(&cursorCounter, 1); 
		return B_OK;
	}
	
	return B_ERROR;
}

//---------------------------------------------------------------------------

BCursorManager::cursor_data::cursor_data()
	: name(B_EMPTY_STRING),
	  animationDelay(100000), // 0.1 second
	  hotspotX(0),
	  hotspotY(0),
	  next(NULL),
	  token(0)
{
	Init();
}

BCursorManager::cursor_data::cursor_data(const char *name, uint8 hotspotX, uint8 hotspotY)
	: name(name),
	  animationDelay(100000), // 0.1 second
	  hotspotX(hotspotX),
	  hotspotY(hotspotY),
	  next(NULL),
	  token(0)
{
	Init();
}

BCursorManager::cursor_data::~cursor_data()
{
	for (uint32 i = 0; i < MAX_CURSOR_ANIM_FRAMES; i++) {
		if (rawData[i] != NULL) {
			free(rawData[i]);
		}
	}
}

void
BCursorManager::cursor_data::Init()
{
	for (uint32 i = 0; i < MAX_CURSOR_ANIM_FRAMES; i++) {
		rawData[i] = NULL;
	}
	// white
	highColor.red = 255;
	highColor.green = 255;
	highColor.blue = 255;
	highColor.alpha = 255;
	// black
	lowColor.red = 0;
	lowColor.green = 0;
	lowColor.blue = 0;
	lowColor.alpha = 255;
}

bool
BCursorManager::cursor_data::operator==(const cursor_data &cd) const
{
	// compares all public fields, meaning all fields except for "rawData", "next", and "token"
	return ((name == cd.name)
			&& (animationDelay == cd.animationDelay)
			&& (highColor == cd.highColor)
			&& (lowColor == cd.lowColor)
			&& (hotspotX == cd.hotspotX)
			&& (hotspotY == cd.hotspotY));
}

BCursorManager::cursor_data&
BCursorManager::cursor_data::operator=(const cursor_data &cd)
{
	if (this != &cd) {
		name = cd.name;
		animationDelay = cd.animationDelay;
		highColor = cd.highColor;
		lowColor = cd.lowColor;
		hotspotX = cd.hotspotX;
		hotspotY = cd.hotspotY;

		for (uint32 i = 0; i < MAX_CURSOR_ANIM_FRAMES; i++) {
			if (rawData[i] != NULL) {
				free(rawData[i]);
			}
			rawData[i] = NULL;
		}
		// these fields don't get copied
		next = NULL;
		token = 0;
	}
	return *this;
}



//---------------------------------------------------------------------------

enum {
	PNG_CHECK_BYTES = 8
};

static void read_png_data(png_structp png_ptr,
						  png_bytep data,
						  png_uint_32 length)
{
	BDataIO* io = (BDataIO*)png_get_io_ptr(png_ptr);
	ssize_t amount = io->Read(data, length);
	if( amount < B_OK ) png_error(png_ptr, "Read Error");
}

static BBitmap* read_png_image(BDataIO* stream)
{
	png_byte buf[PNG_CHECK_BYTES];
	
	// Make sure there is a header
	if( stream->Read(buf, PNG_CHECK_BYTES) != PNG_CHECK_BYTES ) return 0;
	
	// Check the header
	if( png_sig_cmp(buf, 0, PNG_CHECK_BYTES) != 0 ) return 0;
	
	png_structp png_ptr;
	png_infop info_ptr;
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return 0;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp) NULL);
		return 0;
	}
	
	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		return 0;
	}
	
	png_set_read_fn(png_ptr, stream, read_png_data);
	png_set_sig_bytes(png_ptr, PNG_CHECK_BYTES);
	
	png_read_info(png_ptr, info_ptr);
	
	// Set up color space conversion
	png_byte color_type(png_get_color_type(png_ptr, info_ptr));
	png_byte bit_depth(png_get_bit_depth(png_ptr, info_ptr));

	if (bit_depth <= 8) {
		png_set_expand(png_ptr);
		png_set_packing(png_ptr);
	}

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);

	if (bit_depth > 8)
		png_set_strip_16(png_ptr);
	
	png_set_bgr(png_ptr);

	if (!(color_type & PNG_COLOR_MASK_COLOR))
		png_set_gray_to_rgb(png_ptr);

	if (!(color_type & PNG_COLOR_MASK_ALPHA))
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	
	png_read_update_info(png_ptr, info_ptr);
	png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
	png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
	
	// Create bitmap
	BBitmap* bitmap =
		new BBitmap(BRect(0, 0, width-1, height-1), 0, B_RGBA32);
	
	png_bytep* rows = new png_bytep[height];
	for( size_t i=0; i<height; i++ ) {
		rows[i] = ((png_bytep)bitmap->Bits()) + i*bitmap->BytesPerRow();
	}
	png_read_image(png_ptr, rows);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	
	return bitmap;
}

uint32
rgbcolor_to_int(rgb_color col)
{
	uint32 r = col.red;
	r = r << 16;
	uint32 g = col.green;
	g = g << 8;
	uint32 b = col.blue;
	return (r | g | b);
}

void*
Load32x32PNGCursor(const char *filename, uint16 hotspotX, uint16 hotspotY,
	rgb_color high, rgb_color low)
{
	BFile file(filename, O_RDONLY);
	BBitmap *bitmap = NULL;
	char *cursorData;
	uchar *inData;

	if (file.InitCheck() != B_OK) {
		PRINT(("Couldn't open file\n"));
		goto error1;
	}
	
	bitmap = read_png_image(&file);
	if (bitmap == NULL) {
		PRINT(("failed to read image\n"));
		goto error1;
	}

	if (bitmap->Bounds().Width() != 31 || bitmap->Bounds().Height() != 31) {
		PRINT(("Bitmap is not 32 x 32 (%f x %f)\n", bitmap->Bounds().Width(),
			bitmap->Bounds().Height()));
		goto error2;
	}

	cursorData = (char*) calloc(268, 1);
	if (cursorData == 0) {
		PRINT(("out of memory\n"));
		goto error2;
	}
	
	cursorData[0] = 32;			// 32 pixels per side
	cursorData[1] = 1;			// depth
	cursorData[2] = hotspotX;
	cursorData[3] = hotspotY;

	inData = (uchar*) bitmap->Bits();
	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {
			uchar *pixel = &inData[x * 4 + y * bitmap->BytesPerRow()];
			if (pixel[3] == 255) {
				// This pixel is not transparent
				cursorData[132 + y * 4 + x / 8] |= (1 << (7 - (x % 8)));

				// set pixel color
				if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
					cursorData[4 + y * 4 + x / 8] |= (1 << (7 - (x % 8)));
			}
		}
	}

	// set the colors
	*((uint32*)(((uint8*)cursorData)+260)) = rgbcolor_to_int(low);
	*((uint32*)(((uint8*)cursorData)+264)) = rgbcolor_to_int(high);

	delete bitmap;
	PRINT(("read cursor \"%s\"\n", path));

#if CURSOR_MANAGER_DEBUG
	fprintf(stderr, "BCursorManager: successfully loaded cursor '%s'\n", filename);
#endif

	return (void*)cursorData;

error2:
	delete bitmap;
error1:
	fprintf(stderr, "BCursorManager: failed to load cursor '%s'\n", filename);
	return 0;
}
