//
//	An LBX file is a "collection" file, as it contains multiple
//	resources.  This addon gets invoked for file paths that don't map
//	to real files.  It groks the path to figure out where the real
//	data file is, then pulls the specific instance out of it.
//


#include <Autolock.h>
#include <Bitmap.h>
#include <BitmapCollection.h>
#include <Content.h>
#include <Debug.h>
#include <File.h>
#include <ObjectList.h>
#include <Resource.h>
#include <ResourceCache.h>
#include <SmartArray.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <StringBuffer.h>
#include <URL.h>
#include <View.h>

// #undef PRINT
// #define PRINT(_x) printf _x

using namespace Wagner;

const char *kContainerFileName = "images.lbx";

struct ContainerInfo {
	BString path;
	int refCount;
	BBitmapCollection *collection;
};

class LbxContentInstance : public ContentInstance {
public:
	LbxContentInstance(Content*, GHandler*, BBitmap*);
	virtual status_t Draw(BView *into, BRect exposed);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *flags);

private:
	BBitmap *fBitmap;
};

class LbxContent : public Content {
public:
	LbxContent(void* handle);
	~LbxContent();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual	size_t GetMemoryUsage();
	virtual bool IsInitialized();

private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler*,
		const BMessage&);
	static ContainerInfo* LookupContainerInfo(const char *path);
	static void ExpandPathVariables(StringBuffer &out, StringBuffer &inBuf);

	BBitmap *fBitmap;
	ContainerInfo *fContainer;
	static BLocker fLock;
	static BObjectList<ContainerInfo> fContainers;
};

BLocker LbxContent::fLock("LBX cache lock");
BObjectList<ContainerInfo> LbxContent::fContainers(20, true);

LbxContentInstance::LbxContentInstance(Content *content, GHandler *handler, BBitmap *bitmap)
	:	ContentInstance(content, handler),
		fBitmap(bitmap)
{
}

status_t LbxContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	if (!fBitmap) {
		*width = 0;
		*height = 0;
		*flags = 0;
		return B_ERROR;
	}
		
	BRect bounds = fBitmap->Bounds();
	*width = static_cast<int32>(bounds.Width()) + 1;
	*height = static_cast<int32>(bounds.Height()) + 1;
    *flags = 0;      	
	return B_OK;
}

status_t LbxContentInstance::Draw(BView *into, BRect exposed)
{
	PRINT(("LbxContentInstance::Draw, fBitmap = %p\n", fBitmap));
	if (!fBitmap)
		return B_ERROR;
	into->SetDrawingMode(B_OP_ALPHA);
	into->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	into->DrawBitmapAsync(fBitmap, fBitmap->Bounds(), FrameInParent());
	return B_OK;
}

LbxContent::LbxContent(void *handle)
	:	Content(handle),
		fBitmap(0),
		fContainer(0)
{
}

LbxContent::~LbxContent()
{
	BAutolock lock(&fLock);
	delete fBitmap;
	if (fContainer && --fContainer->refCount == 0) {
		delete fContainer->collection;
		fContainer->collection = 0;	
	}
}

ssize_t LbxContent::Feed(const void *buffer, ssize_t count, bool done)
{
	return count;
}

size_t LbxContent::GetMemoryUsage()
{
	return 0;
}

bool LbxContent::IsInitialized()
{
	return fBitmap != 0;
}

ContainerInfo* LbxContent::LookupContainerInfo(const char *path)
{
	for (int i = 0; i < fContainers.CountItems(); i++) 
		if (fContainers.ItemAt(i)->path == path)
			return fContainers.ItemAt(i);

	ContainerInfo *file = new ContainerInfo;
	file->path = path;
	file->collection = 0;
	file->refCount = 0;
	fContainers.AddItem(file);
	return file;
}

status_t LbxContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&)
{
	if (fBitmap == 0) {
		char path[4096];
		char name[255] = "";
		StringBuffer urlString;
		GetResource()->GetURL().AppendTo(urlString);
		StringBuffer expanded;
		ExpandPathVariables(expanded, urlString);
		URL newURL(expanded.String());
		newURL.GetUnescapedPath(path, 4096);



		for (char *c = path + strlen(path); c > path; c--) {
			if (*c == '/') {
				strcpy(name, c + 1);
				*c = '\0';
				break;
			}
		}

		strcat(path, "/");
		strcat(path, kContainerFileName);

		PRINT(("Try to open container file \"%s\"\n", path));
		BAutolock lock(&fLock);
		fContainer = LookupContainerInfo(path);
		if (fContainer->collection == 0) {
			PRINT(("Creating collection object\n"));
			BFile file(path, B_READ_ONLY);
			if (file.InitCheck() < 0) {
				PRINT(("Error %s opening file [%s]\n", strerror(file.InitCheck()), name));
				return file.InitCheck();
			}
			
			off_t fileLength;
			file.GetSize(&fileLength);
			PRINT(("Size of file is %Ld\n", fileLength));
			
			status_t error;
			BBitmapCollection *collection = new BBitmapCollection(&file, fileLength);
			if ((error = collection->InitCheck()) < 0) {
				PRINT(("Error creating bitmap collection\n"));
				delete collection;
				return error;
			}
			
			fContainer->collection = collection;
		}
	
		PRINT(("Found container, refCount = %d\n", fContainer->refCount));
		fContainer->refCount++;
		status_t error = B_NO_INIT;
		fBitmap = fContainer->collection->GetBitmap(name);
		if (!fBitmap || ((error = fBitmap->InitCheck()) != B_OK))
		{ // Make sure the bitmap was created
			PRINT(("*** Creation of bitmap [%s] %p FAILED (%s)\n", name, fBitmap, strerror(error)));
			delete fBitmap;
			fBitmap = 0;
			return error;
		}
		
		PRINT(("Created bitmap %p\n", fBitmap));
	}

	*outInstance = new LbxContentInstance(this, handler, fBitmap);
	return B_OK;
}

void LbxContent::ExpandPathVariables(StringBuffer &out, StringBuffer &inBuf)
{
	const char *in = inBuf.String();
	while (*in) {
		if (*in == '$') {
			in++;
			char variableName[B_PATH_NAME_LENGTH];
			int i = 0;
			int len = 0;
			while (*in && *in != '/') {
				if (len++ < B_PATH_NAME_LENGTH)
					variableName[i++] = *in;
			
				in++;
			}

			variableName[i] = '\0';
			
			out.Append(configSettings[variableName].String().String());
		} else
			out << *in++;
	}
}

class LbxContentFactory : public ContentFactory {
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 // BE AWARE: Any changes you make to these identifiers should
		 // also be made in the 'addattr' command in the makefile
		 // and also in FileProtocol.cpp
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.Lbx");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char*,
								   const char*)
	{
		return new LbxContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0)
		return new LbxContentFactory;

	return 0;
}
