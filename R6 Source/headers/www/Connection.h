#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <DataIO.h>

class Connection : public BDataIO {
public:
	Connection() { }
	~Connection() { }
	virtual void Abort() = 0;
	virtual bool HasUnreadData() = 0;
};

#if ENABLE_CONNECTION_TRACE
	#define CONNECTION_NORMAL_COLOR "\e[0m"
	#define CONNECTION_STATUS_COLOR "\e[0;35m"

	#define CONNECTION_TRACE(x) printf x
#else
	#define CONNECTION_TRACE(x) ;
#endif

#endif // _CONNECTION_H
