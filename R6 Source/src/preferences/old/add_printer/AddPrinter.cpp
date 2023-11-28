/*
	
	AddPrinter.cpp
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#ifndef HELLO_WORLD_H
#include "AddPrinter.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <OS.h>
#include <PrintJob.h>

main()
{	

	APApplication *myApplication;

	myApplication = new APApplication();
	
	delete(myApplication);
	return(0);
}

APApplication::APApplication()
		  		  : BApplication("application/x-vnd.Be-ADPR")
{
	run_add_printer_panel();
}
