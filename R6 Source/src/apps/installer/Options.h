// ------------------------------------------------------------------
// Options.h
//
//   UI stuff for the Installer options.
//
//   by Nathan Schrenk (nschrenk@be.com)
// ------------------------------------------------------------------

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <map>
#include <stdio.h>
#include <string.h>
#include <List.h>
#include <ScrollView.h>
#include <StringView.h>
#include <View.h>
#include "OptionCheckBox.h"


// Draws a grouping header
class GroupHeaderView : public BView
{
public:
	GroupHeaderView(char *name);
	~GroupHeaderView();
	virtual void Draw(BRect update);
	virtual void GetPreferredSize(float *width, float *height);
private:
	char *name;
};

// Represents a group of options
class OptionGroup : public BList
{
public:
	OptionGroup(char *name)
	{
		this->name = strdup(name);
		header_view = new GroupHeaderView(name);
	}
	virtual ~OptionGroup() {
//		delete header_view;
		free(name);
	}
	BView *header_view; // the view class used as the header for this group
	char *name;
};

// comparator class for char*
struct ltstr 
{ 
  bool operator()(const char* s1, const char* s2) const 
  {
//	printf("operator() called...\n");
//	if (!s1 || !s2) {
//		printf("s1 or s2 is NULL!!!\n");
//		return 0;
//	}
//	else {
//		printf("s1 = %s, s2 = %s\n", s1, s2);
		return strcmp(s1, s2) < 0;
//	}
  } 
};

// Container that organizes a list of OptionCheckBox objects
class OptionView : public BView
{
public:
	OptionView(BRect frame);
	virtual ~OptionView();
	
	void AddOption(OptionCheckBox *option);
	void ArrangeOptions();
	void Clear();
	void SetEnabled(bool enabled);
	OptionalPackage **GetSelectedOptions();
	off_t CalculateSelectedSize();
	// BView hook functions
	virtual void AttachedToWindow();
	virtual void TargetedByScrollView(BScrollView *scroller);
	virtual void GetPreferredSize(float *width, float *height);
	virtual void Draw(BRect updateRect);
//	virtual void MakeFocus(bool focused);
//	virtual void KeyDown(const char *bytes, int32 numBytes);
		
private:
	BScrollView *scroll;
	void AdjustScrollBars();
	bool arranged;  // whether or not the option views have been arranged
	bool compensated_for_scrollview_brokenness;
	float pref_height;
	map<const char*, OptionGroup*,  ltstr> groups;  // groups of options
};


#endif // _OPTIONS_H_
