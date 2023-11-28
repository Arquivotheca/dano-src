
#ifndef	_SUPPORT2_BYTESTREAM_H
#define	_SUPPORT2_BYTESTREAM_H

#include <support2/IByteStream.h>
#include <support2/SupportDefs.h>
#include <support2/Binder.h>

namespace B {
namespace Support2 {

/*-----------------------------------------------------------------*/

class LByteInput : public LInterface<IByteInput>
{
	public:
		virtual	status_t		Link(const IBinder::ptr &to, const BValue &bindings);
		virtual	status_t		Unlink(const IBinder::ptr &from, const BValue &bindings);
		virtual	status_t		Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out);
		virtual	status_t		Transact(uint32 code, BParcel& data, BParcel* reply = NULL, uint32 flags = 0);
		
	protected:
		inline					LByteInput() { }
		inline virtual			~LByteInput() { }
		
		virtual	status_t		Told(BValue &in);
		virtual	status_t		Asked(const BValue &outBindings, BValue &out);
		virtual	status_t		Called(	BValue &in,
										const BValue &outBindings,
										BValue &out);
	
	private:
								LByteInput(const LByteInput& o);
};

/*-----------------------------------------------------------------*/

class LByteOutput : public LInterface<IByteOutput>
{
	public:
		virtual	status_t		Link(const IBinder::ptr &to, const BValue &bindings);
		virtual	status_t		Unlink(const IBinder::ptr &from, const BValue &bindings);
		virtual	status_t		Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out);
		virtual	status_t		Transact(uint32 code, BParcel& data, BParcel* reply = NULL, uint32 flags = 0);
		
	protected:
		inline					LByteOutput() { }
		inline virtual			~LByteOutput() { }
		
		virtual	status_t		Told(BValue &in);
		virtual	status_t		Asked(const BValue &outBindings, BValue &out);
		virtual	status_t		Called(	BValue &in,
										const BValue &outBindings,
										BValue &out);
	
	private:
								LByteOutput(const LByteOutput& o);
};

/*-----------------------------------------------------------------*/

class LByteSeekable : public LInterface<IByteSeekable>
{
	public:
		virtual	status_t		Link(const IBinder::ptr &to, const BValue &bindings);
		virtual	status_t		Unlink(const IBinder::ptr &from, const BValue &bindings);
		virtual	status_t		Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out);
		virtual	status_t		Transact(uint32 code, BParcel& data, BParcel* reply = NULL, uint32 flags = 0);
		
	protected:
		inline					LByteSeekable() { }
		inline virtual			~LByteSeekable() { }
		
		virtual	status_t		Told(BValue &in);
		virtual	status_t		Asked(const BValue &outBindings, BValue &out);
		virtual	status_t		Called(	BValue &in,
										const BValue &outBindings,
										BValue &out);
	
	private:
								LByteSeekable(const LByteSeekable& o);
};

/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_BYTESTREAM_H */
