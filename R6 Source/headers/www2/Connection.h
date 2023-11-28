#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <support2/ByteStream.h>

namespace B {
namespace WWW2 {

using namespace B::Support2;

class Connection : public LByteInput, public LByteOutput
{
	public:
		Connection() { }
		~Connection() { }
		
		virtual bool HasUnreadData() = 0;
};

#if ENABLE_CONNECTION_TRACE
	#define CONNECTION_NORMAL_COLOR "\e[0m"
	#define CONNECTION_STATUS_COLOR "\e[0;35m"

	#define CONNECTION_TRACE(x) printf x
#else
	#define CONNECTION_TRACE(x) ;
#endif

} } // namespace B::WWW2

#endif // _CONNECTION_H
