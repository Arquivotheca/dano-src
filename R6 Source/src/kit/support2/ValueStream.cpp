
#include <support2/ValueStream.h>

#include <support2/StdIO.h>
#include <support2_p/BinderKeys.h>

namespace B {
namespace Support2 {

using namespace B::Private;

/*-----------------------------------------------------------------*/

class RValueInput : public RInterface<IValueInput>
{
	public:

		RValueInput(IBinder::arg o) : RInterface<IValueInput>(o) {};

		virtual	status_t Read(BValue *in)
		{
			const BValue result = Remote()->Invoke(g_keyRead);
			bout << "RValueInput::Read(): " << result << endl;
			status_t status;
			if (result.IsDefined()) {
				*in = result[g_keyValue];
				if (in->IsDefined()) status = B_OK;
				else {
					const BValue statusV = result[g_keyStatus];
					status = statusV.IsDefined() ? statusV.AsInt32() : B_ERROR;
				}
			} else {
				status = B_ERROR;
			}
			return status;
		}
};

class RValueOutput : public RInterface<IValueOutput>
{
	public:

		RValueOutput(IBinder::arg o) : RInterface<IValueOutput>(o) {};

		virtual	status_t Write(const BValue &out)
		{
			bout << "RValueOutput::Write(" << out << ")" << endl;
			const BValue result = Remote()->Invoke(BValue(g_keyValue, out), g_keyWrite);
			bout << "RValueOutput: write result = " << result << endl;
			const BValue status = result[g_keyStatus];
			return status.IsDefined() ? status.AsInt32() : B_OK;
		}

		virtual status_t End()
		{
			bout << "RValueOutput::End()" << endl;
			const BValue result = Remote()->Invoke(g_keyEnd);
			const BValue status = result[g_keyStatus];
			return status.IsDefined() ? status.AsInt32() : B_OK;
		}
};

/*-----------------------------------------------------------------*/

B_IMPLEMENT_META_INTERFACE(ValueInput)
B_IMPLEMENT_META_INTERFACE(ValueOutput)

/*-----------------------------------------------------------------*/

status_t
LValueInput::Link(const IBinder::ptr &to, const BValue &bindings)
{
	return BBinder::Link(to, bindings);
}

status_t
LValueInput::Unlink(const IBinder::ptr &from, const BValue &bindings)
{
	return BBinder::Unlink(from, bindings);
}

status_t
LValueInput::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	return BBinder::Effect(in, inBindings, outBindings, out);
}

status_t
LValueInput::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	return BBinder::Transact(code, data, reply, flags);
}

status_t 
LValueInput::Told(BValue &/*in*/)
{
	return B_OK;
}

status_t 
LValueInput::Asked(const BValue &/*outBindings*/, BValue &/*out*/)
{
	return B_OK;
}

status_t
LValueInput::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	if (in[g_keyRead].IsDefined()) {
		BValue result;
		const status_t status = Read(&result);
		bout << "LValueInput::Read(): " << result << endl;
		out += outBindings *
			( status >= B_OK
				? BValue(g_keyValue, result)
				: BValue(g_keyStatus, BValue::Int32(status))
			);
	}
	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
LValueOutput::Link(const IBinder::ptr &to, const BValue &bindings)
{
	return BBinder::Link(to, bindings);
}

status_t
LValueOutput::Unlink(const IBinder::ptr &from, const BValue &bindings)
{
	return BBinder::Unlink(from, bindings);
}

status_t
LValueOutput::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	return BBinder::Effect(in, inBindings, outBindings, out);
}

status_t
LValueOutput::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	return BBinder::Transact(code, data, reply, flags);
}

status_t 
LValueOutput::Told(BValue &/*val*/)
{
	return B_OK;
}

status_t 
LValueOutput::Asked(const BValue &/*outBindings*/, BValue &/*out*/)
{
	return B_OK;
}

status_t
LValueOutput::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue value;
	if ((value=in[g_keyWrite][g_keyValue]).IsDefined()) {
		bout << "LValueOutput::Write(" << value << ")" << endl;
		const status_t status = Write(value);
		if (status < B_OK) out += outBindings * BValue(g_keyStatus, BValue::Int32(status));
	} else if ((value=in[g_keyEnd]).IsDefined()) {
		bout << "LValueOutput::End()" << endl;
		const status_t status = End();
		if (status < B_OK) out += outBindings * BValue(g_keyStatus, BValue::Int32(status));
	}
	return B_OK;
}

} } // namespace B::Support2
