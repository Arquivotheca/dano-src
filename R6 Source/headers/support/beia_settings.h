#ifndef BEIA_SETTINGS_H
#define BEIA_SETTINGS_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

void get_setting(const char *domain, const char *name, char *value, size_t max);
status_t get_nth_setting(const char *domain, size_t index, char *name, size_t namemax, char *value, size_t valmax);
void set_setting(const char *domain, const char *name, const char *value);

#ifdef __cplusplus
}
#endif


#endif /* BEIA_SETTINGS_H */
