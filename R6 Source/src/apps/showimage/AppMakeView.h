/*	AppMakeView.h
 *	The workhorse function that calls Datatypes.lib to find out what a file really is,
 *	and if it knows the base format, creates an appropriate display view.
 */

#include "TranslationKit.h"
#include "String.h"
class BPositionIO;

/*	The main work function */
extern long AppMakeView(
				BPositionIO *		inStream,
				BView * &		outView,
				BRect &			outExtent,
				const char *	hintMIME,
				BString &		outDataInfo,
				translator_id &	outTranslator,
				uint32 &		outFormat);



/*	Helper functions */
extern long AppMakeTextView(
				BView * &		outView,
				BRect &			outExtent,
				BPositionIO &		stream,
				translator_info &		info);
extern long AppMakeBitmapView(
				BView * &		outView,
				BRect &			outExtent,
				BPositionIO &		stream,
				translator_info &		info);
extern long AppMakePictureView(
				BView * &		outView,
				BRect &			outExtent,
				BPositionIO &		stream,
				translator_info &		info);
extern long AppMakeMediaView(
				BView * &		outView,
				BRect &			outExtent,
				BPositionIO &		stream,
				translator_info &		info);
