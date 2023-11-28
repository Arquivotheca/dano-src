/******************************************************************************
/
/	File:			StringIO.h
/
/	Description:	ITextOutput interface for creating strings.
/
/	Copyright 2001, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_STRINGIO_H
#define	_SUPPORT2_STRINGIO_H

#include <support2/TextStream.h>
#include <support2/PositionIO.h>
#include <support2/MemoryStore.h>

namespace B {
namespace Support2 {

/*-------------------------------------------------------------------*/
/*------- BStringIO Class -------------------------------------------*/

class BStringIO : public BMallocStore, public BPositionIO, public BTextOutput
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BStringIO);

							BStringIO();
		virtual				~BStringIO();
		
		const char *		String();
		size_t				StringLength() const;
		void				Reset();
		void				PrintAndReset(ITextOutput::arg io);
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

}} // namespace B::Support2

#endif /* _SUPPORT2_STRINGIO_H */
