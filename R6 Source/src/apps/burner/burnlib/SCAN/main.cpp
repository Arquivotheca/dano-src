/*
	Attempt at finding drives and determing type and if a cd-burner.
		--Joseph D. Groover, JR
*/


#include <stdio.h>
#include <String.h>
#include "DriveDetect.h"

int main()
{
	printf("\t\tGroover CD R(W) Drive Detection\n");
	printf("Running through possible devices...\n");
	
	IDriveDetect*det = new IDriveDetect();
	det->BeginSearch();
	
	return(0);
}



