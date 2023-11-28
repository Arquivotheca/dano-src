/*
	MSNTransport.h
*/
#ifndef MSN_TRANSPORT_H
#define MSN_TRANSPORT_H

#include "IMTransport.h"

class MSNTransport : public IMTransport {
	public:
								MSNTransport();
		virtual					~MSNTransport();
	
		virtual status_t		Login(const char *username);
		virtual status_t		Logout();
	
	protected:

	private:

};

#endif
// End of MSNProtocol.h
