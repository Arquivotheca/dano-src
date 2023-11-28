#include <Be.h>
#if (!__INTEL__)
#include "DatatypesAddOn.h"

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define LIBNAME "libdatatypes.so"

_DatatypesCallbacks		DATACallbacks;

int	DATALoadCallbacks()
{
	/*	check for previous load	*/
	status_t err = B_OK;
	image_id the_image;
	if (DATACallbacks.image < 0)
		return DATACallbacks.image;		// error
	if (DATACallbacks.image > 0)
		return 0;						// ok
	/*	try to load		*/
	//DATACallbacks.image = load_add_on(LIBNAME);
	//if (DATACallbacks.image < 0)
	{	/*	load through the shared lib path	*/
		char * env = getenv("LIBRARY_PATH");
		char * end, * temp, * str;
		if (!env)
			return DATACallbacks.image;
		env = strdup(env);
		temp = env;
		while (1)
		{
			end = strchr(temp, ':');
			if (end)
				*end = 0;
			if (!strncmp(temp, "%A/", 3))
			{
				str = (char *)malloc(1024);
				if (!str)
				{
					free(env);
					return B_NO_MEMORY;
				}
				if (getcwd(str, 1023) == NULL)
				{
					free(env);
					free(str);
					return errno;
				}
				strcat(str, temp+2);	/*	include slash we know is there	*/
			}
			else
			{
				str = (char *)malloc(strlen(temp)+40);
				if (!str)
				{
					free(env);
					return B_NO_MEMORY;
				}
				strcpy(str, temp);
			}
			strcat(str, "/");
			strcat(str, LIBNAME);
			DATACallbacks.image = load_add_on(str);
			free(str);
			if (DATACallbacks.image > 0)
				break;
			if (!end)
				break;
			temp = end+1;
		}
		free(env);
		if (DATACallbacks.image < 0)
			return DATACallbacks.image;
	}
	the_image = DATACallbacks.image;
	DATACallbacks.image = -1;
#define SYM(x) if (0 > (err = get_image_symbol(the_image, #x, B_SYMBOL_TYPE_TEXT, (void **)&DATACallbacks.x))) return err
	/*	load all the symbols required by the library	*/
	SYM(DATAVersion);
	SYM(DATAInit);
	SYM(DATAShutdown);
	SYM(DATAIdentify);
	SYM(DATAGetHandlers);
	SYM(DATAGetAllHandlers);
	SYM(DATAGetHandlerInfo);
	SYM(DATAGetInputFormats);
	SYM(DATAGetOutputFormats);
	SYM(DATATranslate);
	SYM(DATAMakeConfig);
	SYM(DATAGetConfigMessage);
	
	
	DATACallbacks.image = the_image;
	return B_OK;
}
#endif
