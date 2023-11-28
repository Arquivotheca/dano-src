/*
 *		Command line app:   'eject'
 *				Duplication attempt from R5 command app
 *				Some differences may be had, as I am not too 
 *				certain how the original works
*/	

#include <Drivers.h>
#include <stdio.h>

int main(int argc, char**argv)
{
	const char* which_device = "/dev/disk/floppy/raw";		
	if (argc > 1)
		{
			if (!strcmp(argv[1], "--devices"))
			{
				return(0);
			}
			else if (!strcmp(argv[1], "--help"))
			{
				return(0);
			}
			else
			{
				which_device = argv[1];
			}
		}
	
	int file = open(which_device, 0);
	if (file < 0)
		{	printf("Invalid device %s\n", which_device);
			return(-1);
		}
	int ret = ioctl(file, B_EJECT_DEVICE, 0);
	switch(ret)
		{
			case EBUSY:
				{	printf("%s is in use -- device not ejected.\n", which_device);
					return(-2);	// gained return results by watching behaviour of original
					break;
				}
		}
	
		if (ret < 0)
			{	perror(which_device);
				return(-3);
			}	

	return(0);
}



