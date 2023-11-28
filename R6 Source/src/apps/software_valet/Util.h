#ifndef _UTIL_H_
#define _UTIL_H_

// Util.h

class BDirectory;
class BView;
class BWindow;

#include <GraphicsDefs.h>
#include <Message.h>
long doError(const char *msg);

long doError(const char *format, const char *subtext);

long doError(const char *format,
			 const char *subtext,
			 const char *button1,
			 const char *button2 = NULL,
			 const char *button3 = NULL);

long doFatal(const char *format, const char *subtext = NULL);

extern const rgb_color light_gray_background;
extern const rgb_color label_red;

void DrawHSeparator(float left, float right, float v, BView *view);

void PositionWindow(BWindow *w,float horizFrac, float vertFrac);
void PositionWindow(BWindow *, BPoint, float, float);

bool TryActivate(BMessenger &wind);

#define nel(x) (sizeof(x)/sizeof((*x)))

char *PathForDir(BDirectory *dir,char *buf, char *bufMax);
status_t MergeMessage(BMessage &dst, BMessage &src);

inline void ReplaceString(BMessage *m, const char *f, const char *d)
{
	if (d == NULL) d = B_EMPTY_STRING;
	if (m->ReplaceString(f,d) < B_OK)
		m->AddString(f,d);
}

inline void ReplaceBool(BMessage *m, const char *f, bool d)
{
	if (m->ReplaceBool(f,d) < B_OK)
		m->AddBool(f,d);	
}

inline void ReplaceInt16(BMessage *m, const char *f, int16 d)
{
	if (m->ReplaceInt16(f,d) < B_OK)
		m->AddInt16(f,d);	
}

inline void ReplaceInt32(BMessage *m, const char *f, int32 d)
{
	if (m->ReplaceInt32(f,d) < B_OK)
		m->AddInt32(f,d);	
}

inline void ReplaceInt64(BMessage *m, const char *f, int64 d)
{
	if (m->ReplaceInt64(f,d) < B_OK)
		m->AddInt64(f,d);	
}

inline void ReplaceFloat(BMessage *m, const char *f, float d)
{
	if (m->ReplaceFloat(f,d) < B_OK)
		m->AddFloat(f,d);	
}

inline const char *FindString(BMessage *m, const char *f)
{
	const char *r = m->FindString(f);
	if (!r) return B_EMPTY_STRING;
	return r;
}

#endif
