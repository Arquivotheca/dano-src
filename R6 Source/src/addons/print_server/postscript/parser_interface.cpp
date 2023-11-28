#include "ppd_control.h"
#include "Element.h"
#include <stdio.h>

#define P /* printf */
#define PP // printf

extern "C" {
	void open_group(char *name, char *translation);
	void close_group(char *name);
	void open_ui(char *name, char *translation, char *type);
	void start_invocation(char *name, char *option, char *translation);
	void add_invocation_line(char *invoke);
	void close_ui(char *name);
	void print_everything();
	void imageable_area(char *name, char *translation, char *box);
	void paper_dimension(char *name, char *size);
	void set_language_level(char *lev);
	void set_default(char *name, char *option, char *translation);
	void add_stringval(char *name, char *option, char *translation, char *string);
	void add_ui_constraint(char *key1, char *opt1, char *key2, char *opt2);
	void jcl_open_ui(char *name, char *translation, char *type);
	void jcl_close_ui(char *name);
}

void
open_group(char *name, char *translation)
{
	(PPD::Instance())->AddNewGroup(name, translation);	
}

void
close_group(char *name)
{
	(PPD::Instance())->CloseGroup(name);
}

void
open_ui(char *name, char *translation, char *type)
{
	(PPD::Instance())->AddNewUI(name, translation, type);	
}

void
start_invocation(char *name, char *option, char *translation)
{
	PP("in start_invocation(%s, %s, %s)\n", name, option, translation);
	(PPD::Instance())->AddNewInvocation(name, option, translation);
}

void add_invocation_line(char *invoke)
{
	// add invocation fragment to current invocation
	(PPD::Instance())->AddInvocationFragment(invoke);
}

void set_default(char *name, char *option, char *translation)
{
	(PPD::Instance())->MakeDefault(name, option, translation);
}


void close_ui(char *name)
{
	(PPD::Instance())->CloseUI(name);
}

void print_everything()
{
	//ppd.PrintUIs();
}

void add_stringval(char *name, char *option, char *translation, char *string)
{
	(PPD::Instance())->AddStringVal(name, option, translation, string);
}

void add_ui_constraint(char *key1, char *opt1, char *key2, char *opt2)
{
	(PPD::Instance())->AddUiConstraint(key1, opt1, key2, opt2);
}


void imageable_area(char *name, char *translation, char *box)
{
	// not in use...
}

void paper_dimension(char *name, char *size)
{
	// not in use..
}

void set_language_level(char *lev)
{
	int level;
	sscanf(lev, "%d", &level);
	(PPD::Instance())->LanguageLevel(level);
}

void
jcl_open_ui(char *name, char *translation, char *type)
{
	open_ui(name, translation, type);
	(PPD::Instance())->MarkCurrentUIAsJCL();
}

void jcl_close_ui(char *name)
{
	close_ui(name);
}


