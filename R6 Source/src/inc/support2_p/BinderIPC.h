
#ifndef _SUPPORT2_BINDERIPC_H_
#define _SUPPORT2_BINDERIPC_H_

#include <support2/SupportDefs.h>
#include <support2/Binder.h>

namespace B {
namespace Private {

using namespace B::Support2;

/**************************************************************************************/

class RBinder : public IBinder
{
	public:

										RBinder(int32 handle);
		virtual							~RBinder();

				int32					Handle() { return m_handle; };

		virtual	BValue					Inspect(const BValue &which, uint32 flags = 0);
		virtual	status_t				Link(const IBinder::ptr &to, const value &bindings);
		virtual	status_t				Unlink(const IBinder::ptr &from, const value &bindings);
		virtual	status_t				Effect(const value &in, const value &inBindings, const value &outBindings, value *out);

		virtual	status_t				Transact(	uint32 code,
													BParcel& data,
													BParcel* reply = NULL,
													uint32 flags = 0);
		
		virtual	BBinder*				LocalBinder();
		virtual	RBinder*				RemoteBinder();
		
	private:

		virtual	status_t				Acquired(const void* id);
		virtual	status_t				Released(const void* id);
		virtual	status_t				AcquireAttempted(const void* id);

				int32					m_handle;
};

/**************************************************************************************/

} }	// namespace B::Private

#endif	/* _SUPPORT2_BINDERIPC_H_ */
