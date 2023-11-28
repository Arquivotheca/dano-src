// ------------------------------------------------------------------
// OptionCheckBox.h
//
//   Shows a checkbox for an optional installation package.
//
//   by Nathan Schrenk (nschrenk@be.com)
// ------------------------------------------------------------------

#include <CheckBox.h>
#include "OptionalPackage.h"

const uint32 OPTION_MOUSED_OVER = 'opmo';
const uint32 OPTION_CHECKED = 'opcb';

// prototype
void convert_size_to_string(off_t size, char *buf);

class OptionCheckBox :  public BCheckBox
{
public:
	OptionCheckBox(OptionalPackage *pkg, BMessage *msg);
	virtual ~OptionCheckBox();
	OptionalPackage *GetPackage();
	
protected:
	virtual void Draw(BRect update);
	virtual void GetPreferredSize(float *width, float *height);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	
private:
	OptionalPackage *option;
	char *size_str; // option->size converted to a string
	
//	OptionCheckBox(const OptionCheckBox &foo) : BCheckBox(foo) {} // unusable
};
