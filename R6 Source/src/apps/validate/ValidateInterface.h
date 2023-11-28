
#if !defined(ValidateInterface_h)
#define ValidateInterface_h

#include <SupportDefs.h>

class BMessenger;
class Test;
struct ValidateInterface;

struct stage_t {
	int32 stage;
	const char * name;
	const char * short_name;
	Test * (*func)();
	BView* (*make_view_func)(const BRect &, ValidateInterface *);
	int32 next_stage;
	int option;
};

//	Ignore is like Pass except different.

enum {
	kUnknown = 0,
	kPass = 1,
	kFail = 2,
	kIgnore = 3
};

struct ValidateInterface
{
	size_t size;
	void (*action)(const char * fmt, ...);
	const char * (*get_setting)(const char * name, char * buf, int size);
	int64 (*get_setting_value)(const char * name, int64 def = 0);
	const char * (*get_current_directory)();
	const char * (*get_test_directory)();
};


#endif	//	ValidateInterface_h

