
#include "CMapEngine.h"

CMapOperators	CMapOps;

CMapEngine::CMapEngine(PDFFontEncoding *encoding) :
	fEncoding(encoding)
{
	printf("CMapEngine::CMapEngine()\n");
}


CMapEngine::~CMapEngine()
{
}

ssize_t 
CMapEngine::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	// this ain't going to work
	return -1;
}

ssize_t 
CMapEngine::Write(BPrivate:: PDFObject *obj)
{
	//printf("CMapEngine::UnknownKeyword(%x)\n", obj);
	if (obj->IsKeyword()) {
		DispatchKeyword(obj);
	}
	else {
		fStack.push_back(obj);
	}
	return ObjectSink::OK;
}

status_t 
CMapEngine::UnknownKeyword(PDFObject *obj)
{
		const char *key = obj->GetCharPtr();
		
		if (key == CMapOps.beginbfchar)
			beginbfcharFn();
		else if (key == CMapOps.endbfchar)
			endbfcharFn();
		else if (key == CMapOps.beginbfrange)
			beginbfrangeFn();
		else if (key == CMapOps.endbfrange)
			endbfrangeFn();
		else if (key == CMapOps.begincidchar)
			begincidcharFn();
		else if (key == CMapOps.endcidchar)
			endcidcharFn();
		else if (key == CMapOps.begincidrange)
			begincidrangeFn();
		else if (key == CMapOps.endcidrange)
			endcidrangeFn();
		else if (key == CMapOps.begincmap)
			begincmapFn();
		else if (key == CMapOps.endcmap)
			endcmapFn();
		else if (key == CMapOps.begincodespacerange)
			begincodespacerangeFn();
		else if (key == CMapOps.endcodespacerange)
			endcodespacerangeFn();
		else if (key == CMapOps.beginnotdefchar)
			beginnotdefcharFn();
		else if (key == CMapOps.endnotdefchar)
			endnotdefcharFn();
		else if (key == CMapOps.beginnotdefrange)
			beginnotdefrangeFn();
		else if (key == CMapOps.endnotdefrange)
			endnotdefrangeFn();
		else if (key == CMapOps.beginrearrangedfont)
			beginrearrangedfontFn();
		else if (key == CMapOps.endrearrangedfont)
			endrearrangedfontFn();
		else if (key == CMapOps.beginusematrix)
			beginusematrixFn();
		else if (key == CMapOps.endusematrix)
			endusematrixFn();
		else if (key == CMapOps.usecmap)
			usecmapFn();
		else if (key == CMapOps.usefont)
			usefontFn();
		else
			UnknownOp(key);
			
		obj->Release();
}

void 
CMapEngine::EatStack(uint32 count)
{
	if (!count || (count > fStack.size())) count = fStack.size();

	while (count--) {
		PDFObject *obj = fStack.back();
		fStack.pop_back();
		obj->Release();
	}
}

void 
CMapEngine::UnknownOp(const char *op)
{
	printf("unknown op: %s stack size: %d\n", op, fStack.size());
	EatStack();
}



void 
CMapEngine::beginbfcharFn()
{
	// int beginbfchar	int number of input code -> charcode or charname pairs
	get_num_items(BF_CHAR);
}

void 
CMapEngine::endbfcharFn()
{
	// lastSrcCode lastDstCode endbfchar
	// lastSrcCode /lastDstName endbfchar
	check_state(BF_CHAR, 2);
		
	while (fNumItems > 0) {
		fNumItems--;
		PDFObject *dst = PopStack();
		PDFObject *src = PopStack();
		
		//if (src->IsNumber()) {
			if (dst->IsNumber() || dst->IsString())
				fEncoding->ReplaceUnicodeWith(dst->GetInt32(), src->GetInt32());
			else if (dst->IsName())
				fEncoding->ReplaceUnicodeWith(dst->GetCharPtr(), src->GetInt32());
		//}
		dst->Release();
		src->Release();
	}
}

void 
CMapEngine::beginbfrangeFn()
{
	// int beginfrangeFn
	get_num_items(BF_RANGE);
}

void 
CMapEngine::endbfrangeFn()
{
	// srcCodeLo srcCodeHi dstCodeLo endbfrange
	// srcCodeLo srcCodeHi [dstCharnameLo ... dstCharNameHi]
	
	check_state(BF_RANGE, 3);

	while (fNumItems > 0) {
		fNumItems--;
		PDFObject *dst = PopStack();
		PDFObject *srcHi = PopStack();
		PDFObject *srcLo = PopStack();
	
		int32 srcCodeLo = srcLo->GetInt32();
		int32 srcCodeHi = srcHi->GetInt32();
	
		if (dst->IsArray()) {
			object_array *dstNames = dst->Array();
			fEncoding->ReplaceUnicodeRangeWith(srcCodeLo, srcCodeHi, dstNames);
//			for (int ix = 0; ix < (srcCodeHi - srcCodeLo); ix++) {
//				fEncoding->ReplaceUnicodeWith(dstLo + ix, dstNames[ix]);
//			}
		}
		else {
			int32 dstLo = dst->GetInt32();
			fEncoding->ReplaceUnicodeRangeWith(srcCodeLo, srcCodeHi, dstLo);
//			for (int ix = 0; ix < srcCodeHi - srcCodeLo; ix++) {
//				fEncoding->ReplaceUnicodeWith(dstLo + ix, srcLo + ix);
//			} 
		}
	
		dst->Release();
		srcHi->Release();
		srcLo->Release();
	}
}

void 
CMapEngine::begincidcharFn()
{
	// int begincidcharFn
	get_num_items(CID_CHAR);
}

void 
CMapEngine::endcidcharFn()
{
	check_state(CID_CHAR, 2);
	EatStack(fNumItems * 2);
	fNumItems = 0;
}

void 
CMapEngine::begincidrangeFn()
{
	// int begincidrangeFn
	get_num_items(CID_RANGE);
}

void 
CMapEngine::endcidrangeFn()
{
	check_state(CID_RANGE, 3);
	EatStack(fNumItems * 3);
	fNumItems = 0;
}

void 
CMapEngine::begincmapFn()
{
	printf("begincmapFn\n");
}

void 
CMapEngine::endcmapFn()
{
	printf("endcmapFn\n");
}

void 
CMapEngine::begincodespacerangeFn()
{
	get_num_items(CODE_RANGE);
}

void 
CMapEngine::endcodespacerangeFn()
{
	check_state(CODE_RANGE, 2);	
	while (fNumItems > 0) {
		fNumItems--;
		PDFObject *end = PopStack();
		PDFObject *begin = PopStack();
		
		printf("codespacerange: 0x%04x -> 0x%04x\n", begin->GetInt32(), end->GetInt32());
		
		end->Release();
		begin->Release();	
	}
}

void 
CMapEngine::beginnotdefcharFn()
{
	get_num_items(NOTDEF_CHAR);
}

void 
CMapEngine::endnotdefcharFn()
{
	check_state(NOTDEF_CHAR, 2);

	while (fNumItems > 0) {
		fNumItems--;
		PDFObject *dstCID = PopStack();
		PDFObject *srcCode = PopStack();
		
		printf("notdefchar: 0x%04x -> 0x%04x\n", srcCode->GetInt32(), dstCID->GetInt32());
		
		dstCID->Release();
		srcCode->Release();
	}
}

void 
CMapEngine::beginnotdefrangeFn()
{
	get_num_items(NOTDEF_RANGE);
}

void 
CMapEngine::endnotdefrangeFn()
{
	check_state(NOTDEF_RANGE, 3);
	
	while (fNumItems > 0) {
		fNumItems--;
		PDFObject *dstCID = PopStack();
		PDFObject *srcCodeHi = PopStack();
		PDFObject *srcCodeLo = PopStack();
		
		printf("endnotdefrange: 0x%04x -> 0x%04x = 0x%04x\n",
			srcCodeLo->GetInt32(), srcCodeHi->GetInt32(), dstCID->GetInt32());		
				
		dstCID->Release();
		srcCodeHi->Release();
		srcCodeLo->Release();
	}
}

void 
CMapEngine::beginrearrangedfontFn()
{
	EatStack(2);
}

void 
CMapEngine::endrearrangedfontFn()
{
	EatStack();
}

void 
CMapEngine::beginusematrixFn()
{
	EatStack(1);
}

void 
CMapEngine::endusematrixFn()
{
	EatStack(1);
}

void 
CMapEngine::usecmapFn()
{
	PDFObject *cmapName = PopStack();
	printf("!!!!!!!!!!!! usecmap %s called! ------------\n", cmapName->GetCharPtr());
	cmapName->Release();
}

void 
CMapEngine::usefontFn()
{
	PDFObject *fontID = PopStack();
	if (fontID->GetInt32() != 0)
		printf("usefont: fontID != 0\n");
	fontID->Release();
}

void 
CMapEngine::get_num_items(int32 state)
{
	// the stack should be 1 deep
	if (fStack.size() != 1)
		printf("get_num_items: state:%d stack is %d!!!\n", state, fStack.size());

	if (fNumItems != 0)
		printf("get_num_items: fNumItems: %lu\n", fNumItems);

	PDFObject *obj = fStack.back();
	if (obj->IsNumber()) {
		fStack.pop_back();
		int32 num = obj->GetInt32();
		fNumItems = num;
		fState = state;
		obj->Release();
	}
	
}

void
CMapEngine::check_state(uint32 state, uint32 multiplier)
{
	if (fState != state)
		printf("expected state: %ld actual state: %ld\n", state, fState);
	if (fStack.size() != fNumItems * multiplier)
		printf("state: %ld expected stack size: %ld stack size: %ld\n",
					state, fNumItems * multiplier, fStack.size());
}

CMapOperators::CMapOperators()
{
	#define intern(x) x = PDFObject::StaticAtom(#x)
		intern(beginbfchar);
		intern(endbfchar);
		intern(beginbfrange);
		intern(endbfrange);
		intern(begincidchar);
		intern(endcidchar);
		intern(begincidrange);
		intern(endcidrange);
		intern(begincmap);
		intern(endcmap);
		intern(begincodespacerange);
		intern(endcodespacerange);
		intern(beginnotdefchar);
		intern(endnotdefchar);
		intern(beginnotdefrange);
		intern(endnotdefrange);
		intern(beginrearrangedfont);
		intern(endrearrangedfont);
		intern(beginusematrix);
		intern(endusematrix);
		intern(usecmap);
		intern(usefont);
		intern(def);
	#undef intern(x)
}



