
#ifndef	_SUPPORT2_VALUESTREAM_H
#define	_SUPPORT2_VALUESTREAM_H

#include <support2/IValueStream.h>
#include <support2/SupportDefs.h>
#include <support2/Binder.h>

namespace B {
namespace Support2 {

/*-----------------------------------------------------------------*/

class LValueInput : public LInterface<IValueInput>
{
	public:
		virtual	status_t		Link(const IBinder::ptr &to, const BValue &bindings);
		virtual	status_t		Unlink(const IBinder::ptr &from, const BValue &bindings);
		virtual	status_t		Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out);
		virtual	status_t		Transact(uint32 code, BParcel& data, BParcel* reply = NULL, uint32 flags = 0);
		
	protected:
		inline					LValueInput() { }
		inline virtual			~LValueInput() { }
		
		virtual	status_t		Told(BValue &in);
		virtual	status_t		Asked(const BValue &outBindings, BValue &out);
		virtual	status_t		Called(	BValue &in,
										const BValue &outBindings,
										BValue &out);
	
	private:
								LValueInput(const LValueInput& o);
};

/*-----------------------------------------------------------------*/

class LValueOutput : public LInterface<IValueOutput>
{
	public:
		virtual	status_t		Link(const IBinder::ptr &to, const BValue &bindings);
		virtual	status_t		Unlink(const IBinder::ptr &from, const BValue &bindings);
		virtual	status_t		Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out);
		virtual	status_t		Transact(uint32 code, BParcel& data, BParcel* reply = NULL, uint32 flags = 0);
		
	protected:
		inline					LValueOutput() { }
		inline virtual			~LValueOutput() { }
		
		virtual	status_t		Told(BValue &in);
		virtual	status_t		Asked(const BValue &outBindings, BValue &out);
		virtual	status_t		Called(	BValue &in,
										const BValue &outBindings,
										BValue &out);
	
	private:
								LValueOutput(const LValueOutput& o);
};

/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_BYTESTREAM_H */
