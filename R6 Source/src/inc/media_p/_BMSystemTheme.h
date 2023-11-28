/*	_BMSystemTheme.h	*/


#if !defined(_B_M_SYSTEM_THEME_H)
#define _B_M_SYSTEM_THEME_H

#if !defined(_VIEW_H)
 #include <View.h>
#endif

#if !defined(_CONTROL_H)
#include <Control.h>
#endif

#if !defined(_MESSAGE_FILTER_H)
#include <MessageFilter.h>
#endif

#if !defined(_CONTROL_THEME_H)
 #include "MediaTheme.h"
#endif

class ParamData;
class DataList;
class RowList;

class _BMSystemTheme :
	public BMediaTheme
{
public:
		_BMSystemTheme();
		~_BMSystemTheme();

virtual	BControl * MakeControlFor(
				BParameter * control);

protected:

virtual	BView * MakeViewFor(
				BParameterWeb * web,
				const BRect * hintRect = NULL);
private:
		BView * MakeViewFor(
				BParameterGroup* group,
				const BRect * hintRect = NULL);
		bool	ParameterIsHidden(BParameter* parameter) const;
};



enum EControlCommands
{
	kControlChanged='CCHG',
	kValueChanged='VCHG'
};


/*
	Interface: BSetValueFilter
	
	This is the special filter installed into the window for
	setting values on controls.  Why use a filter?  Because that
	way we don't have to sub-class BView just to handle messages.
*/

class BSetValueFilter : public BMessageFilter
{
public:
		BSetValueFilter();
		
		filter_result Filter(BMessage *msg, BHandler **target);
};


#endif /* _B_M_SYSTEM_THEME_H */

