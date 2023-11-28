/*
 	Settings API

	Copyright (C) 2000 Be Incorporated.  All Rights Reserved.
 */


#ifndef H_SETTINGS
#define H_SETTINGS

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************
   BeIA settings functions
   accesses and modifies settings files for BeIA
   file is /boot/home/config/settings/beia-<domain>, containing name=value pairs
 */ 
void get_setting(const char *domain, const char *name, char *value, size_t max);
status_t get_nth_setting(const char *domain, size_t index, char *name, size_t namemax, char *value, size_t valmax);
void set_setting(const char *domain, const char *name, const char *value);

/**************************/


/**************************
   Heirarchial settings file functions

	manages configuration heirarchies in B_USER_CONFIG_DIRECTORY.
	
	Specific config files are named <B_USER_CONFIG_DIRECTORY>/<domain>/<component>
	
	Data stored in these files is in a tree of the form:
	
	foo = bar
	level1 {
		foo = bar
		baz = quuz/quuz/quuz
		level2a {
			eeny = "meeny miny mo"
		}
		
		level2b {
			level3 {
				poot = woot
			}
		}
	
	}

*/

/*
 * get or put the config file. 
 */
status_t get_config(const char *domain, const char *component, void **cookie);
status_t put_config(void *cookie);

/*
 * lock or unlock (reentrantly) the config file.  All the following
 * operations require the config file to be locked.
 */
status_t lock_config(void *cookie);
status_t unlock_config(void *cookie);

/*
 * count the number of child levels of the current level
 */
size_t get_config_level_count(void *cookie);

/*
 * get the nth child level name of this level
 */
status_t get_nth_config_level(void *cookie, int n, char *name, size_t namelen);

/*
 * change the level of the heirarchy you are browsing.  CONFIG_ROOT browses the root level.
 * CONFIG_PARENT browses the parent level of the current one. CONFIG_CHILD causes the browse
 * level to change to that specified by the path specified by strings subsequently passed in
 * varargs.  for example, from above,
 *
 *
 * lock_config(foo);
 * browse_config_level(myfile, CONFIG_ROOT); // necessary because some other thread may
 *                                           // have changed the current level.
 * browse_config_level(myfile, CONFIG_CHILD, "level1", "level2b", "level3", NULL);
 *                                           // the NULL is REQUIRED.  
 * [...read values, etc...]
 * unlock_config(foo);
 */

enum {
	CONFIG_ROOT = 1,     /* browse the root level of the config file heirarchy */
	CONFIG_PARENT,       /* browse the parent of the current level */
	CONFIG_CHILD         /* browse a descendant of the currentl level */
};
status_t browse_config_level(void *cookie, int browse, ...);

/*
 * get the value of name at the current level.
 */
status_t get_config_value(void *cookie, const char *name, char *value, size_t valuelen, const char *defval);


/*
 * count the name=value pairs at the current level
 */
size_t get_config_name_count(void *cookie);

/*
 * get the nth name=value at this level
 */
status_t get_nth_config_name(void *cookie, int n, char *name, size_t namelen, char *value, size_t valuelen);

/*
 * set name=value at this level. 
 */
status_t set_config_value(void *cookie, const char *name, char *value);

/*
 * remove a name=value at this level.
 */
status_t remove_config_value(void *cookie, const char *name);

/*
 * Add a child to the current level.
 */
status_t add_config_level(void *cookie, const char *name);

/*
 * Remove a child from the current level.
 */
status_t remove_config_level(void *cookie, const char *name);

				 
/***************************/

#ifdef __cplusplus
}
#endif

#endif
