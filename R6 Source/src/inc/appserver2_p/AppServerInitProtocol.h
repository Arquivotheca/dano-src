
#ifndef	_APPSERVER2_INITPROTOCOL_H_
#define	_APPSERVER2_INITPROTOCOL_H_

namespace B {
namespace AppServer2 {

enum InitMessages {
	HELLO				=	1000, /* send to picasso to get the server port id */
	NEW_TERM			=	1001, /* server port msg		 */
	COUT				=	1002, /* terminal port message	*/ 
	NEW_MW				=	1003, /* server port msg		 */
	LINE				=	1004,
	TITLE				=	1005,
	DEATH_CERTIFICATE	=	1006,
	ANY_SCREENS			=	1007,
	INIT_SESSION		=	0x1009,
	GR_GET_VERSION		=	0x100A,
	GR_CLOSEAPP			=	0x100b,
	UPDATE				=	0x100c,
	SET_ORIGIN			=	0x100d,
	CLOSE				=	0x100e,
	ROUTE_EVENTS		=	0x100f
};

} }	// namespace B::AppServer2

using namespace B::AppServer2;

#endif 	/* _APPSERVER2_INITPROTOCOL_H_ */
