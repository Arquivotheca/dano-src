
#if !defined(settings_h)
#define settings_h


#include <SupportDefs.h>	//	for int64


#if defined(__cplusplus)
extern "C"
{
#endif


status_t read_settings(const char * path);
status_t write_settings(const char * path);

status_t set_setting_value(const char * name, int64 number);
status_t set_setting(const char * name, const char * value);
status_t remove_setting(const char * name);

int64 get_setting_value(const char * name, int64 def = 0);
//	If you supply a buffer, it will be returned un-touched if there is no setting,
//  (and the return value will be NULL)
//	else the setting will be copied into it.
//	If you don't supply a buffer, a thread-unsafe pointer to the settings value
//	will be returned; any other call into settings may invalidate this pointer.
const char * get_setting(const char * name, char * buffer = NULL, int size = 0);

const char* get_settings_file();

#if defined(__cplusplus)
};
#endif


#endif	//	settings_h
