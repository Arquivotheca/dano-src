#include "DescriptionEngine.h"
#include "PDFKeywords.h"

#ifndef NDEBUG
#include <stdio.h>
#include <OS.h>
//#define DFN(a) if (PDF_##a != fCurrentKey) { char buffer[64]; sprintf(buffer, #a"Fn() mismatch: %ld vs. %d\n", fCurrentKey, PDF_##a); debugger(buffer); }
#define DFN(a) { printf("DescriptionEngine::"#a"Fn() : %d\n", PDF_##a); }
#else
#define DFN(a)
#endif

#ifndef NDEBUG
static void TwoHex(uint8 b, uint8 *buf)
{
	buf++;
	uint8 h = b & 0x0f;
	if (h > 9) h += 'A' - 10;
	else h += '0';
	*buf-- = h;
	b >>= 4;
	h = b & 0x0f;
	if (h > 9) h += 'A' - 10;
	else h += '0';
	*buf-- = h;
};
static void DumpChunk(uint32 offset, uint8 *inbuf, int32 size)
{
	uint8 buf[16 * 4 + 2];
	while (size)
	{
		memset(buf, ' ', sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		for (int byte = 0; byte < 16 && size; byte++)
		{
			TwoHex(*inbuf, buf + (byte * 3));
			uint8 code = *inbuf++;
			buf[3 * 16 + 1 + byte] = ((code >= ' ') && (code <= 127)) ? code : '.';
			size--;
		}
		printf("%08lx  %s\n", offset, buf);
		offset += 16;
	}
	
}
#endif

void ExpandNames(PDFObject *o)
{
	if (o->IsDictionary() || o->IsArray())
	{
		object_array *oa = o->Array();
		object_array::iterator oai;
		PDFObject *tmp, *newTmp;
		for (oai = oa->begin(); oai < oa->end(); oai++)
		{
			tmp = *oai;
			newTmp = 0;
			if (tmp->IsName())
			{
				const char *name = tmp->GetCharPtr();
				if (name == PDFAtom.A85)
					newTmp = PDFObject::makeName(PDFAtom.ASCII85Decode);
				else if (name == PDFAtom.AHx)
					newTmp = PDFObject::makeName(PDFAtom.ASCIIHexDecode);
				else if (name == PDFAtom.BPC)
					newTmp = PDFObject::makeName(PDFAtom.BitsPerComponent);
				else if (name == PDFAtom.CCF)
					newTmp = PDFObject::makeName(PDFAtom.CCITTFaxDecode);
				else if (name == PDFAtom.CS)
					newTmp = PDFObject::makeName(PDFAtom.ColorSpace);
				else if (name == PDFAtom.DCT)
					newTmp = PDFObject::makeName(PDFAtom.DCTDecode);
				else if (name == PDFAtom.D)
					newTmp = PDFObject::makeName(PDFAtom.Decode);
				else if (name == PDFAtom.DP)
					newTmp = PDFObject::makeName(PDFAtom.DecodeParms);
				else if (name == PDFAtom.CMYK)
					newTmp = PDFObject::makeName(PDFAtom.DeviceCMYK);
				else if (name == PDFAtom.G)
					newTmp = PDFObject::makeName(PDFAtom.DeviceGray);
				else if (name == PDFAtom.RGB)
					newTmp = PDFObject::makeName(PDFAtom.DeviceRGB);
				else if (name == PDFAtom.F)
					newTmp = PDFObject::makeName(PDFAtom.Filter);
				else if (name == PDFAtom.Fl)
					newTmp = PDFObject::makeName(PDFAtom.FlateDecode);
				else if (name == PDFAtom.H)
					newTmp = PDFObject::makeName(PDFAtom.Height);
				else if (name == PDFAtom.IM)
					newTmp = PDFObject::makeName(PDFAtom.ImageMask);
				else if (name == PDFAtom.I)
				{
					if (o->IsArray())
						newTmp = PDFObject::makeName(PDFAtom.Indexed);
					else
						newTmp = PDFObject::makeName(PDFAtom.Interpolate);
				}
				else if (name == PDFAtom.LZW)
					newTmp = PDFObject::makeName(PDFAtom.LZWDecode);
				else if (name == PDFAtom.RL)
					newTmp = PDFObject::makeName(PDFAtom.RunLengthDecode);
				else if (name == PDFAtom.W)
					newTmp = PDFObject::makeName(PDFAtom.Width);
				if (newTmp)
				{
					// free the old one
					tmp->Release();
					// store the new one, giving it our reference
					*oai = newTmp;
				}
			}
			else if (tmp->IsArray())
				// recurse to pick up abreviated filters and color space names
				ExpandNames(tmp);
		}
	}
}

DescriptionEngine::DescriptionEngine()
	: fWantRawData(false), fPulseInterval(0)
{
}


DescriptionEngine::~DescriptionEngine()
{
	// clean up object stack
	eat_stack();
	// no need to clean up index stack
}

void
DescriptionEngine::eat_stack(size_t count)
{
	if (!count || (count > fStack.size())) count = fStack.size();

	while (count--) {
		PDFObject *obj = fStack.back();
		fStack.pop_back();
		obj->Release();
	}
}

typedef enum { start, white_space, E, I, done } EIStateMachineStates;

ssize_t
ProbeForEI(const uint8 *buffer, ssize_t length, int *this_state)
{
	EIStateMachineStates next_state = start;
	ssize_t origLength = length;
	ssize_t EIstart = 0;

	//printf("Searching for \" EI \" in chunk:\n"); DumpChunk(0, buffer, length); printf("\n");
	while (length && (*this_state != done))
	{
		// guess a state based on input
		switch (*buffer++)
		{
			case '\0':
			case '\n':
			case '\r':
			case '\f':
			case '\t':
			case ' ':
				next_state = white_space;
				break;
			case 'E':
				next_state = E;
				break;
			case 'I':
				next_state = I;
				break;
			default:
				next_state = start;
				break;
		}
		// run it through the state machine
		switch (next_state)
		{
			case start:
				*this_state = start;
				break;
			case white_space:
			{
				switch (*this_state)
				{
					case I:	// found white space after EI!
						*this_state = done;
						break;
					default:
						*this_state = white_space;
						EIstart = origLength - length;
						break;
				}
			} break;
			case E:
			{
				switch (*this_state)
				{
					case white_space: // E after white space: E
						*this_state = E;
						break;
					default:
						*this_state = start;
						break;
				}
			} break;
			case I:
			{
				switch (*this_state)
				{
					case E: // I after E: I
						*this_state = I;
						break;
					default:
						*this_state = start;
						break;
				}
			} break;
			default:
				break;
		}
		// next byte
		length--;
	}
	// return number of bytes consumed before the EI token
	//printf("orig: %ld, EIstart: %ld, done: %s\n", origLength, EIstart, *this_state == done ? "true" : "false"); fflush(stdout);
	return *this_state == done ? EIstart : origLength;
}


ssize_t 
DescriptionEngine::Write(const uint8 *buffer, ssize_t length, bool )
{
	// the only time we want raw data is with @#$%^ inline images
	// The Plan:
	//   Read data until we see the (\r)\nEI(\r)\n sequence
	if (!fWantRawData) return ObjectSink::WANT_OBJECTS;
	// consume exactly one whitespace charater after the ID operator
	ssize_t IDwhitespace = 0;
	if (fConsumeIDwhitespace)
	{
		buffer++;
		length--;
		fConsumeIDwhitespace = false;
		IDwhitespace++;
	}
	ssize_t consumed = ProbeForEI(buffer, length, &fEIstate);
	// append consumed data to our string buffer
	ssize_t count = consumed;
	while (count--) fString.Append((char)(*buffer++));
	if (consumed != length)
	{
		// no more raw data
		fWantRawData = false;
	}
	return consumed + IDwhitespace;
}

ssize_t 
DescriptionEngine::Write(PDFObject *obj)
{
	if (obj->IsKeyword())
	{
		int32 key = obj->GetInt32();
		//printf("object is keyword %lu\n", key);
		if ((key != PDF_UNKNOWN_KEY) && (key < PDF_R))
		{
#ifndef NDEBUG
			fCurrentKey = key;
#endif
			(this->*(fFnArray[key]))();
			// free up the keyword
			obj->Release();
		}
		else
		{
			UnknownKeyword(obj);
		}
	}
	else
	{
		//printf("pushing object on stack\n");
		fStack.push_back(obj);
	}
	if (fPulseInterval)
	{
		bigtime_t now = system_time();
		if (now >= fNextPulse)
		{
			fNextPulse = now + fPulseInterval;
			Pulse();
		}
	}
	return fWantRawData ? ObjectSink::WANT_RAW_DATA : ObjectSink::OK;
}

void 
DescriptionEngine::Pulse(void)
{
}


void 
DescriptionEngine::UnknownKeyword(PDFObject *keyword)
{
	// discard the unknown keyword and all stack elements
#ifndef NDEBUG
	printf("UnknownKeyword - "); keyword->PrintToStream(); printf("\n");
#endif
	keyword->Release();
	eat_stack();
}


void 
DescriptionEngine::startarrayFn()
{
	// note position on stack of array
	fIndicies.push_back(fStack.end() - fStack.begin());
}

void 
DescriptionEngine::endarrayFn()
{
	// duplicate the entries.  The new array will take our refs.
	object_array::iterator first = fStack.begin() + fIndicies.back();
	PDFObject *array = PDFObject::makeArray(first, fStack.end());
	fStack.erase(first, fStack.end());
	fStack.push_back(array);
	fIndicies.pop_back();
}

void 
DescriptionEngine::startdictFn()
{
	// note position on stack of dictionary
	fIndicies.push_back(fStack.end() - fStack.begin());
}

void 
DescriptionEngine::enddictFn()
{
	endarrayFn();
	fStack.back()->PromoteToDictionary();
}

void 
DescriptionEngine::bFn()
// ( - )
{
	DFN(b);
	// closepath fill and stroke
}

void 
DescriptionEngine::bstarFn()
// ( - )
{
	DFN(bstar);
	// closepath eofill and stroke
}

void 
DescriptionEngine::BFn()
// ( - )
{
	DFN(B);
	// fill and stroke
}

void 
DescriptionEngine::BstarFn()
// ( - )
{
	DFN(Bstar);
	// eofill and stroke
}

void 
DescriptionEngine::BDCFn()
// ( tag properties - )
{
	DFN(BDC);
	// begin dictionary content
	eat_stack(2);
}

void 
DescriptionEngine::BIFn()
{
	DFN(BI);
	// queue up an entry on the index stack
	fIndicies.push_back(fStack.end() - fStack.begin());
}

void 
DescriptionEngine::BMCFn()
// ( tag - )
{
	DFN(BMC);
	// begin marked content
	eat_stack(1);
}

void 
DescriptionEngine::BTFn()
// ( - )
{
	DFN(BT);
	// begin text - initialize text matrix and line matrix
}

void 
DescriptionEngine::BXFn()
// ( - )
{
	DFN(BX);
	// push DONT_NOTIFY on unknown operator reporting stack
}

void 
DescriptionEngine::cFn()
// ( x1 y1 x2 y2 x3 y3 - )
{
	DFN(c);
	// curveto
	eat_stack(6);
}

void 
DescriptionEngine::cmFn()
// ( a b c d e f - )
{
	DFN(cm);
	eat_stack(6);
}

void 
DescriptionEngine::csFn()
// ( cs - )
{
	DFN(cs);
	// setcolorspace (fill)
	eat_stack(1);
}

void 
DescriptionEngine::CSFn()
// ( cs - )
{
	DFN(CS);
	// setcolorspace (stroke)
	eat_stack(1);
}

void 
DescriptionEngine::dFn()
// ( [array] phase - )
{
	DFN(d);
	// set dash
	eat_stack(2);
}

void 
DescriptionEngine::d0Fn()
// ( wx wy - )
{
	DFN(d0);
	// set char width for Type 3
	eat_stack(2);
}

void 
DescriptionEngine::d1Fn()
// ( wx wy llx lly urx ury - )
{
	DFN(d1);
	// setcachedevice
	eat_stack(6);
}

void 
DescriptionEngine::DoFn()
// ( name  - )
{
	DFN(Do);
	// deal with xobject
	eat_stack(1);
}

void 
DescriptionEngine::DPFn()
// ( tag properties - )
{
	DFN(DP);
	// dictionary point (for marked content)
	eat_stack(2);
}

void 
DescriptionEngine::EIFn()
{
	DFN(EI);
	// pop the image description off the stack
	PDFObject *obj = fStack.back();
	fStack.pop_back();
	obj->Release();
	// dump the string buffer
	fString.Clear();
}

void 
DescriptionEngine::EMCFn()
// ( - )
{
	DFN(EMC);
	// end marked content
}

void 
DescriptionEngine::ETFn()
// ( - )
{
	DFN(ET);
	// end of text section
}

void 
DescriptionEngine::EXFn()
// ( - )
{
	DFN(EX);
	// pop uknown operator reporting stack
}

void 
DescriptionEngine::fFn()
// ( - )
{
	DFN(f);
	// fill, non-zero winding rule
}

void 
DescriptionEngine::fstarFn()
// ( - )
{
	DFN(fstar);
	// eofill - fill, even-odd rule
}

void 
DescriptionEngine::FFn()
// ( - )
{
	DFN(F);
	// depricated, do as fFn
	fFn();
}

void 
DescriptionEngine::gFn()
// ( gray - )
{
	DFN(g);
	// setgray (fill)
	eat_stack(1);
}

void 
DescriptionEngine::gsFn()
// ( name - )
{
	DFN(gs);
	// graphics state
	eat_stack(1);
}

void 
DescriptionEngine::GFn()
// ( gray - )
{
	DFN(G);
	// setgray (stroke)
	eat_stack(1);
}

void 
DescriptionEngine::hFn()
// ( - )
{
	DFN(h);
	// closepath
}

void 
DescriptionEngine::IDFn()
{
	DFN(ID);
	// build a dictionary out of the stack entries for decoding the inline image
	// duplicate the entries.  The new array will take our refs.
	object_array::iterator first = fStack.begin() + fIndicies.back();
	PDFObject *array = PDFObject::makeArray(first, fStack.end());
	fStack.erase(first, fStack.end());
	fStack.push_back(array);
	fIndicies.pop_back();
	array->PromoteToDictionary();
	// translate short names into standard ones
	ExpandNames(array);
	// switch into raw data mode for the contents
	fWantRawData = true;
	// clear the string buffer
	fString.Clear();
	// reset EI search state
	fEIstate = start;
	// make sure the probe skips the leading white space
	fConsumeIDwhitespace = true;
}

void 
DescriptionEngine::iFn()
// ( i - )
{
	DFN(i);
	// flatness
	eat_stack(1);
}

void 
DescriptionEngine::jFn()
// ( linejoin - )
{
	DFN(j);
	eat_stack(1);
}

void 
DescriptionEngine::JFn()
// ( linecap - )
{
	DFN(J);
	eat_stack(1);
}

void 
DescriptionEngine::kFn()
// ( c y m k - )
{
	DFN(k);
	// setcymkcolor (fill)
	eat_stack(4);
}

void 
DescriptionEngine::KFn()
// ( c y m k - )
{
	DFN(K);
	// setcymkcolor (stroke)
	eat_stack(4);
}

void 
DescriptionEngine::lFn()
// ( x y - )
{
	DFN(l);
	// lineto
	eat_stack(2);
}

void 
DescriptionEngine::mFn()
// ( x y - )
{
	DFN(m);
	// moveto
	eat_stack(2);
}

void 
DescriptionEngine::MFn()
// ( mitrelimit - )
{
	DFN(M);
	eat_stack(1);
}

void 
DescriptionEngine::MPFn()
// ( tag - )
{
	DFN(MP);
	// mark point
	eat_stack(1);
}

void 
DescriptionEngine::nFn()
// ( - )
{
	DFN(n);
	// newpath
}

void 
DescriptionEngine::PSFn()
// ( string - )
{
	DFN(PS);
	// PostScript, for printing only
	eat_stack(1);
}

void 
DescriptionEngine::qFn()
// ( - )
{
	DFN(q);
	// save graphics state to graphics state stack
}

void 
DescriptionEngine::QFn()
// ( - )
{
	DFN(Q);
	// restore graphics state from gstate stack
}

void 
DescriptionEngine::reFn()
// ( x y width height - )
{
	DFN(re);
	// rectangle
	eat_stack(4);
}

void 
DescriptionEngine::rgFn()
// ( r g b - )
{
	DFN(rg);
	// setrgbcolor (fill)
	eat_stack(3);
}

void 
DescriptionEngine::riFn()
// ( intent - )
{
	DFN(ri);
	// rendering intent
	eat_stack(1);
}

void 
DescriptionEngine::RGFn()
// ( r g b - )
{
	DFN(RG);
	// setrgbcolor (stroke)
	eat_stack(3);
}

void 
DescriptionEngine::sFn()
// ( - )
{
	DFN(s);
	// closepath and stroke
}

void 
DescriptionEngine::scFn()
// ( c1 c2 c3 c4 - )
{
	DFN(sc);
	// setcolor (fill)
	// actually, stack arguments varry by current color space
	eat_stack();
}

void 
DescriptionEngine::scnFn()
// stack usage varries by color space
{
	DFN(scn);
}

void 
DescriptionEngine::shFn()
// ( name - )
{
	DFN(sh);
	// shading fill
	eat_stack(1);
}

void 
DescriptionEngine::SFn()
// ( - )
{
	DFN(S);
	// stroke
}

void 
DescriptionEngine::SCFn()
// ( c1 c2 c3 c4 - )
{
	DFN(SC);
	// setcolor (stroke)
	// actually, stack arguments varry by current color space
	eat_stack();
}

void 
DescriptionEngine::SCNFn()
// stack usage varries by color space
{
	DFN(SCN);
	// setcolor
	// actually, stack arguments varry by current color space
	eat_stack();
}

void 
DescriptionEngine::TcFn()
// ( charSpace - )
{
	DFN(Tc);
	// character spacing
	eat_stack(1);
}

void 
DescriptionEngine::TdFn()
// ( tx ty - )
{
	DFN(Td);
	// start of next line
	eat_stack(2);
}

void 
DescriptionEngine::TDFn()
// ( tx ty - )
{
	DFN(TD);
	// start of next line, set leading
	// equal to: ty TLFn tx ty TdFn
	eat_stack(2);
}

void 
DescriptionEngine::TfFn()
// ( fontname size - )
{
	DFN(Tf);
	// set font name and size
	eat_stack(2);
}

void 
DescriptionEngine::TjFn()
// ( string - )
{
	DFN(Tj);
	// show string
	eat_stack(1);
}

void 
DescriptionEngine::TJFn()
// ( [array] - )
{
	DFN(TJ);
	// show, with adjustments
	eat_stack(1);
}

void 
DescriptionEngine::TLFn()
// ( leading - )
{
	DFN(TL);
	// set leading
	eat_stack(1);
}

void 
DescriptionEngine::TmFn()
// ( a b c d x y - )
{
	DFN(Tm);
	// set text matrix
	eat_stack(6);
}

void 
DescriptionEngine::TrFn()
// ( rendering_mode - )
{
	DFN(Tr);
	// set text rendering mode
	eat_stack(1);
}

void 
DescriptionEngine::TsFn()
// ( rise - )
{
	DFN(Ts);
	// set text rise
	eat_stack(1);
}

void 
DescriptionEngine::TwFn()
// ( wordSpace - )
{
	DFN(Tw);
	// set word spacing
	eat_stack(1);
}

void 
DescriptionEngine::TzFn()
// ( scale - )
{
	DFN(Tz);
	// horizontal scaling
	eat_stack(1);
}

void 
DescriptionEngine::TstarFn()
// ( - )
{
	DFN(Tstar);
	// start of next line
}

void 
DescriptionEngine::vFn()
// ( x2 y2 x3 y3 - )
{
	DFN(v);
	// alternate curveto
	eat_stack(4);
}

void 
DescriptionEngine::wFn()
// ( linewidth - )
{
	DFN(w);
	eat_stack(1);
}

void 
DescriptionEngine::WFn()
// ( - )
{
	DFN(W);
	// clip
}

void 
DescriptionEngine::WstarFn()
// ( - )
{
	DFN(Wstar);
	// eoclip
}

void 
DescriptionEngine::yFn()
// ( x1 y1 x3 y3 - )
{
	DFN(y);
	// alterant curveto
	eat_stack(4);
}

void 
DescriptionEngine::tickFn()
// ( string - )
{
	DFN(tick);
	// move to next line and show string
	// same as: T* string Tj
	eat_stack(1);
}

void 
DescriptionEngine::ticktickFn() // aka double-quote
// ( aw ac string - )
{
	DFN(ticktick);
	eat_stack(3);
}

DescriptionEngine::engine_func DescriptionEngine::fFnArray[] = { 
	&DescriptionEngine::bFn, &DescriptionEngine::bstarFn, &DescriptionEngine::BFn, &DescriptionEngine::BstarFn, &DescriptionEngine::BDCFn,
	&DescriptionEngine::BIFn, &DescriptionEngine::BMCFn, &DescriptionEngine::BTFn, &DescriptionEngine::BXFn, &DescriptionEngine::cFn, &DescriptionEngine::cmFn,
	&DescriptionEngine::csFn, &DescriptionEngine::CSFn, &DescriptionEngine::dFn, &DescriptionEngine::d0Fn, &DescriptionEngine::d1Fn, &DescriptionEngine::DoFn, &DescriptionEngine::DPFn,
	&DescriptionEngine::EIFn, &DescriptionEngine::EMCFn, &DescriptionEngine::ETFn, &DescriptionEngine::EXFn, &DescriptionEngine::fFn,
	&DescriptionEngine::fstarFn, &DescriptionEngine::FFn, &DescriptionEngine::gFn, &DescriptionEngine::gsFn, &DescriptionEngine::GFn, &DescriptionEngine::hFn, &DescriptionEngine::iFn,
	&DescriptionEngine::IDFn, &DescriptionEngine::jFn, &DescriptionEngine::JFn, &DescriptionEngine::kFn, &DescriptionEngine::KFn, &DescriptionEngine::lFn, &DescriptionEngine::mFn,
	&DescriptionEngine::MFn, &DescriptionEngine::MPFn, &DescriptionEngine::nFn, &DescriptionEngine::PSFn, &DescriptionEngine::qFn, &DescriptionEngine::QFn,
	&DescriptionEngine::reFn, &DescriptionEngine::rgFn, &DescriptionEngine::riFn, &DescriptionEngine::RGFn, &DescriptionEngine::sFn,
	&DescriptionEngine::scFn, &DescriptionEngine::scnFn, &DescriptionEngine::shFn, &DescriptionEngine::SFn, &DescriptionEngine::SCFn, &DescriptionEngine::SCNFn,
	&DescriptionEngine::TcFn, &DescriptionEngine::TdFn, &DescriptionEngine::TDFn, &DescriptionEngine::TfFn, &DescriptionEngine::TjFn, &DescriptionEngine::TJFn, &DescriptionEngine::TLFn,
	&DescriptionEngine::TmFn, &DescriptionEngine::TrFn, &DescriptionEngine::TsFn, &DescriptionEngine::TwFn, &DescriptionEngine::TzFn, &DescriptionEngine::TstarFn,
	&DescriptionEngine::vFn, &DescriptionEngine::wFn, &DescriptionEngine::WFn, &DescriptionEngine::WstarFn, &DescriptionEngine::yFn, &DescriptionEngine::tickFn,
	&DescriptionEngine::ticktickFn, &DescriptionEngine::startarrayFn, &DescriptionEngine::endarrayFn, &DescriptionEngine::startdictFn, &DescriptionEngine::enddictFn
};
