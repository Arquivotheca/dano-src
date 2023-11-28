
#ifndef	_SUPPORT2_STDIO_H
#define	_SUPPORT2_STDIO_H

#include <support2/IByteStream.h>
#include <support2/ITextStream.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

// Raw byte streams for the standard C files.
extern const IByteInput::ptr Stdin;
extern const IByteOutput::ptr Stdout;
extern const IByteOutput::ptr Stderr;

// Formatted text streams for C standard out, standard error,
// and the debug serial port.
extern ITextOutput::ptr bout;
extern ITextOutput::ptr berr;
extern ITextOutput::ptr bser;

/**************************************************************************************/

} } // namespace B::Support2

#endif /* _SUPPORT2_TEXTOSTR_INTERFACE_H */
