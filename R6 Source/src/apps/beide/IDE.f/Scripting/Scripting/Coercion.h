/*	Coercion.h
 *	$Id: Coercion.h,v 1.1 1996/12/04 11:58:37 hplus Exp elvis $
 *	Type coercion of various common data types
 */

#ifndef _COERCION_H
#define _COERCION_H

#include <Entry.h>
#include <GraphicsDefs.h>
#include <List.h>

class BMessage;
class BRect;
class BPoint;

/*	The default coercion handler only extracts data of the right type.
 *	It is automatically created and added by the Coercions class.
 */
class CoercionHandler
//	public BObject
{
public:
								CoercionHandler();
virtual							~CoercionHandler();

virtual	bool					CanCoerce(
									unsigned long		fromType,
									unsigned long		toType);
virtual	bool					DoCoerce(
									void * &			outData,	//	return block allocated with malloc()
									long &				outSize,
									unsigned long		outType,
									BMessage *			message,
									const char *		name,
									long				index);
};

/*	Coercions between float, double, long, char etc.
 */
class StandardNumberCoercions :
	public CoercionHandler
{
public:

virtual	bool					CanCoerce(
									unsigned long		fromType,
									unsigned long		toType);
virtual	bool					DoCoerce(
									void * &			outData,	//	return block allocated with malloc()
									long &				outSize,
									unsigned long		outType,
									BMessage *			message,
									const char *		name,
									long				index);
};

/*	Coercions between lists of floats or longs and graphics types
 */
class StandardGraphicsCoercions :
	public CoercionHandler
{
public:

virtual	bool					CanCoerce(
									unsigned long		fromType,
									unsigned long		toType);
virtual	bool					DoCoerce(
									void * &			outData,	//	return block allocated with malloc()
									long &				outSize,
									unsigned long		outType,
									BMessage *			message,
									const char *		name,
									long				index);

};

/*	This converts a variety of items into ASCII or CSTRING text.
 *	Note that it doesn't go the other way, to avoid the "anything to text to anything" problem.
  */
class StandardTextCoercions :
	public CoercionHandler
{
public:

virtual	bool					CanCoerce(
									unsigned long		fromType,
									unsigned long		toType);
virtual	bool					DoCoerce(
									void * &			outData,	//	return block allocated with malloc()
									long &				outSize,
									unsigned long		outType,
									BMessage *			message,
									const char *		name,
									long				index);
};

/*	This is the object that dispatches to installed coercion handlers for 
 *	actually performing coercions.
 */
class Coercions
{
public:
								Coercions();
virtual							~Coercions();

		void					AddCoercionHandler(
									CoercionHandler *	handler);

		bool					GetRef(
									entry_ref &			outRef,
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetDouble(
									double &			outDouble,
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetFloat(
									float &				outFloat,
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetLong(
									long &				outLong,
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetString(
									char * &			outString,	//	call free()
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetRect(
									BRect &				outRect,
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetPoint(
									BPoint &			outPoint,
									BMessage *			message,
									const char *		name,
									long				item = 0);
		bool					GetColor(
									rgb_color &			outColor,
									BMessage *			message,
									const char *		name,
									long				item = 0);

		bool					DoCoerce(
									void * &			outData,	//	call free()
									long &				outDataSize,
									unsigned long		toType,
									BMessage *			message,
									const char *		name,
									long				item = 0);

protected:

		BList					handlers;

		CoercionHandler *		FindHandler(
									unsigned long		inType,
									unsigned long		outType,
									int &				iterator);

};

#endif
