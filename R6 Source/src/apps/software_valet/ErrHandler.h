// ErrHandler.h

#ifndef ERR_HANDLER_H
#define ERR_HANDLER_H

//#include "MyDebug.h"

class ErrHandler {
public:
	class OSErr {	// exception class
	public:
		OSErr(status_t code) : err(code) {};
		status_t err;
	};
	class SizeErr {};	
	
	void operator=(status_t code) {
//		PRINT(("error assignment\n"));
		err = code;
		if (err < B_NO_ERROR) {
			throw OSErr(code);
		}
	};
	void CheckSize(status_t size) {
		if (err != size) {
			throw SizeErr();
		}
	};
	status_t	err;
};

#endif
