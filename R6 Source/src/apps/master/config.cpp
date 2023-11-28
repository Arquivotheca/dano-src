
#include <stdio.h>
#include "master.h"
#include "config.h"
#include "util.h"

config cfg;

/*
config::config()
{
	FILE *f;
	char line[1024];
	char name[256];
	char val[1024];
	
	if ((f = fopen(MY_CONFIG_PATH,"r")) == NULL)
		error_die("could not open config file");

	while (fgets(line,1024,f)) {
		if ((!feof(f)) && (!ferror(f))) {
			name[0] = 0;
			val[0] = 0;
			sscanf(line,"%32s %[^\n]\n",name,val);
			cfg.Add(name,val);
		}
	}
}
*/

