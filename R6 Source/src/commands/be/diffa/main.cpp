//--------------------------------------------------------------------
//	
//	main.cpp
//
//	Written by: Myron W. Walker
//	
//	Copyright 2001 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <fs_attr.h>
#include <Debug.h>
#include "AttributeList.h"

#define ARGS_PER_PASS		2

int command(char *argv[]);

int main(int argc, char *argv[])
{
	int iPass = 0, iArgs = 0;
	int passes = ((argc-1)/ARGS_PER_PASS);
	int argc_s[ARGS_PER_PASS];
	
	int condition = 0;
	
	for(iPass = 0; iPass < passes; iPass++)
	{
		int argbase = (iPass * ARGS_PER_PASS) + 1;
		
		condition = command(&argv[argbase]);
		
		if(condition < 0) break;
	}
	
	if(condition < 0)
	{
		cout << "\nError executing command...\n\n";		
	}
}

//Command always uses minimum number of args to execute on instance of a command
int command(char *argv[])
{
	int condition = 0;
	
//	debugger("command() called...");	
		
	AttributeList attrlistA(argv[0]), attrlistB(argv[1]);
	AttributeList *attrlistR = NULL;

	if((attrlistA.size() > 0) && (attrlistB.size() > 0))
	{ 
		attrlistR = attrlistA.compare(&attrlistB);
	}
	else if(attrlistA.size() > 0)
	{
		attrlistR = new AttributeList(&attrlistA);
	}
	else
	{
		attrlistR = new AttributeList(&attrlistB);
	}
	
	if(attrlistR)
	{
		//Print Out Results
		cout << "\nDifferences found in files...\n"
		     << "     FileA=" << argv[0] << "\n"
		     << "     FileB=" << argv[1] << "\n\n";
		
		delete attrlistR;
	}
	
	return condition;
}

