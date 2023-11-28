
/* Datatypes as add-on */
#include <Datatypes.h>

#ifndef _DATATYPESADDON_H
#define _DATATYPESADDON_H

typedef struct {
	image_id	image;

	const char *(*DATAVersion)(	//	returns version string
				int32 & outCurVersion,	//	current version spoken
				int32 & outMinVersion);	//	minimum version understood

	status_t	(*DATAInit)(		//	establish connection
							const char *		app_signature,
							const char *		load_path = NULL);

	status_t	(*DATAShutdown)();	//	don't want to talk anymore

			//	these functions call through to the translators
			//	when wantType is not 0, will only take into consideration 
			//	handlers that can read input data and produce output data

	status_t	(*DATAIdentify)(	//	find out what something is
							BPositionIO &			inSource,
							BMessage *			ioExtension,
							DATAInfo &			outInfo,
							uint32				inHintType = 0,
							const char *		inHintMIME = NULL,
							uint32				inWantType = 0);

	status_t	(*DATAGetHandlers)(//	find all handlers for a type
							BPositionIO &			inSource,
							BMessage *			ioExtension,
							DATAInfo * &		outInfo,	//	call delete[] on outInfo when done
							int32 &				outNumInfo,
							uint32				inHintType = 0,
							const char *		inHintMIME = NULL,
							uint32				inWantType = 0);

	status_t	(*DATAGetAllHandlers)(//	find all handler IDs
							DATAID * &			outList,//	call delete[] when done
							int32 &				outCount);

	status_t		(*DATAGetHandlerInfo)(//	given a handler, get user-visible info
							DATAID				forHandler,
							const char * &		outName,
							const char * &		outInfo,
							int32 &				outVersion);

			//	note that handlers may choose to be "invisible" to
			//	the public formats, and just kick in when they 
			//	recognize a file format by its data.

	status_t	(*DATAGetInputFormats)(//	find all input formats for handler
							DATAID				forHandler,
							const Format * &	outFormats,//	don't write contents!
							int32 &				outNumFormats);

	status_t	(*DATAGetOutputFormats)(//	find all output formats for handler
							DATAID				forHandler,
							const Format * &	outFormats,//	don't write contents!
							int32 &				outNumFormats);

			//	actually do some work

	status_t	(*DATATranslate)(	//	morph data into form we want
							BPositionIO &			inSource,
							const DATAInfo *	inInfo,//	may be NULL
							BMessage *			ioExtension,
							BPositionIO &			outDestination,
							uint32 				inWantOutType,
							uint32				inHintType = 0,
							const char *		inHintMIME = NULL);

			//	For configuring options of the handler, a handler can support 
			//	creating a view to cofigure the handler. The handler can save 
			//	its preferences in the database or settings file as it sees fit.
			//	As a convention, the handler should always save whatever 
			//	settings are changed when the view is deleted or hidden.

	status_t	(*DATAMakeConfig)(
							DATAID				forHandler,
							BMessage *			ioExtension,
							BView * &			outView,
							BRect &				outExtent);

			//	For saving settings and using them later, your app can get the 
			//	current settings from a handler into a BMessage that you create 
			//	and pass in empty. Pass this message (or a copy thereof) to the 
			//	handler later in a call to DATATranslate to translate using 
			//	those settings.

	status_t	(*DATAGetConfigMessage)(
							DATAID				forHandler,
							BMessage *			ioExtension);
}	_DatatypesCallbacks;


extern	_DatatypesCallbacks		DATACallbacks;
extern	int	DATALoadCallbacks();

#endif
