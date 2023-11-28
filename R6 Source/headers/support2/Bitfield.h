
#ifndef _SUPPORT2_BITFIELD_H_
#define _SUPPORT2_BITFIELD_H_

#include <support2/SupportDefs.h>
#include <malloc.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

class BBitfield
{
	public:

					BBitfield(int32 size=0) : m_bits(NULL), m_numBits(0) { Resize(size); }
		            ~BBitfield() { free(m_bits); }

		void		Resize(int32 bits);

		bool		Test(int32 bit) 	{ return m_bits[bit>>5] & (0x80000000 >> (bit & 0x1F)); }
		void		Set(int32 bit)		{ m_bits[bit>>5] |= (0x80000000 >> (bit & 0x1F)); }
		void		Clear(int32 bit)	{ m_bits[bit>>5] &= ~(0x80000000 >> (bit & 0x1F)); }

		void		Set(int32 start, int32 len);
		void		Clear(int32 start, int32 len);

		int32		FirstSet();
		int32		FirstClear();

	private:

		uint32 *	m_bits;
		int32		m_numBits;
};

/**************************************************************************************/

} } // namespace B::Support2

#endif	/* _SUPPORT2_BITFIELD_H_ */
