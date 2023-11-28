
#ifndef	_SUPPORT2_TEXTSTREAM_H
#define	_SUPPORT2_TEXTSTREAM_H

#include <support2/ITextStream.h>
#include <support2/ByteStream.h>

namespace B {
namespace Support2 {

/*-----------------------------------------------------------------*/

class BTextOutput : public ITextOutput
{
	public:

										BTextOutput(IByteOutput::arg stream);
		virtual							~BTextOutput();

		virtual	void					Print(const char *debugText, int32 len = -1);
		virtual void					BumpIndentLevel(int32 delta);

	protected:

										BTextOutput(IByteOutput *This);
	
		// TO DO: Implement LStorage and RStorage.
		virtual	atom_ptr<IBinder>		AsBinderImpl()			{ return NULL; }
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const	{ return NULL; }
		
	private:

				void					WriteIndent();

				IByteOutput *			m_stream;
				int32					m_currentIndent;
				int32					m_front;
};

/*-----------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_TEXTSTREAM_H */
