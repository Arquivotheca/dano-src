/*
	TPreferences.h
	Storage class for preferences (from a newslatter article I think).
*/

#ifndef TPREFERENCES_H
#define TPREFERENCES_H

#include <Message.h>
#include <StorageKit.h>
#include <Autolock.h>

class TPreferences : public BMessage {
public:
				TPreferences(const char *filename);
				~TPreferences();
	status_t	InitCheck(void);

	status_t	SetBool(const char *name, bool b);
	status_t	SetInt8(const char *name, int8 i);
	status_t	SetInt16(const char *name, int16 i);
	status_t	SetInt32(const char *name, int32 i);
	status_t	SetInt64(const char *name, int64 i);
	status_t	SetFloat(const char *name, float f);
	status_t	SetDouble(const char *name, double d);
	status_t	SetString(const char *name, const char *string);
	status_t	SetPoint(const char *name, BPoint p);
	status_t	SetRect(const char *name, BRect r);

	// these Get's lock, so are safe to use from multiple threads
	status_t	GetBool(const char *name, bool *b);
	status_t	GetInt8(const char *name, int8 *i);
	status_t	GetInt16(const char *name, int16 *i);
	status_t	GetInt32(const char *name, int32 *i);
	status_t	GetInt64(const char *name, int64 *i);
	status_t	GetFloat(const char *name, float *f);
	status_t	GetDouble(const char *name, double *d);
	status_t	GetString(const char *name, const char **string);
	status_t	GetPoint(const char *name, BPoint *p);
	status_t	GetRect(const char *name, BRect *r);
private:
	BLocker		fLock;
	BPath		fPath;
	status_t	fStatus;
};

#endif
