// JapaneseIMContent.h

#ifndef JAPANESE_IM_PLUGIN_H
#define JAPANESE_IM_PLUGIN_H

#include <View.h>
#include <Content.h>

using namespace Wagner;

class JapaneseIMContentInstance : public ContentInstance {
public:
						JapaneseIMContentInstance(Content *content,
												  GHandler *handler,
												  const BMessage& params);
	virtual				~JapaneseIMContentInstance();
	virtual status_t	AttachedToView(BView *view, uint32 *contentFlags);
	virtual status_t	DetachedFromView();
	virtual status_t	GetSize(int32 *width, int32 *height, uint32 *flags);
	virtual	status_t	FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);

private:
	BView*	GetIMView(int32 mode);

	BRect	fFrame;
	BView*	fView;
};


class JapaneseIMContent : public Content {
public:
						JapaneseIMContent(void* handle);
	virtual				~JapaneseIMContent();
	virtual ssize_t		Feed(const void *buffer, ssize_t bufferLen,
							 bool done=false);
	virtual size_t		GetMemoryUsage();
	virtual	bool		IsInitialized();

private:
	virtual status_t	CreateInstance(ContentInstance **outInstance,
									   GHandler *handler, const BMessage&);

	friend class JapaneseIMContentInstance;
};

class JapaneseIMContentFactory : public ContentFactory
{
public:
	virtual void		GetIdentifiers(BMessage* into);
	virtual Content*	CreateContent(void* handle,
									  const char* mime,
									  const char* extension);
};

#endif // JAPANESE_IM_PLUGIN_H_
