/* pdf_utils.h */

#ifndef _pdf_utils_h_
#define _pdf_utils_h_

#include <SupportDefs.h>
#include "Object2.h"

class RC4codec;

namespace BPrivate {
class Tokenizer;
class PDFDocument;

PDFObject *		pdf_get_next_object(Tokenizer *lex, PDFDocument *doc, RC4codec *decoder = 0);
PDFObject *		pdf_make_array(Tokenizer *lex, PDFDocument *doc, RC4codec *decoder = 0);
PDFObject *		pdf_make_dictionary(Tokenizer *lex, PDFDocument *doc, RC4codec *decoder = 0);

struct OpaqueBPositionIO : public PDFOpaque {
		OpaqueBPositionIO(BPositionIO *io);
virtual	~OpaqueBPositionIO();

		BPositionIO *m_io;
};

};

#endif