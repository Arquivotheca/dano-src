
#ifndef _CMAP_ENGINE_H_
#define _CMAP_ENGINE_H_

#include "ArrayBuilder.h"
#include "Object2.h"
#include "PDFFontEncoding.h"

class CMapEngine : public ArrayBuilder
{
	public:

		enum {
							BF_CHAR,
							BF_RANGE,
							CID_CHAR,
							CID_RANGE,
							CODE_RANGE,
							NOTDEF_CHAR,
							NOTDEF_RANGE
		} cmap_state;

							CMapEngine(PDFFontEncoding *encoding);
							~CMapEngine();
		
		virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);
		virtual ssize_t		Write(BPrivate::PDFObject *obj);
		virtual status_t	UnknownKeyword(PDFObject *o);

	inline PDFObject *		PopStack() { PDFObject * obj = fStack.back(); fStack.pop_back(); return obj;};
	inline void				PushStack(PDFObject *obj) { fStack.push_back(obj); };
		void				EatStack(uint32 count = 0);				
		void				UnknownOp(const char *op);
	private:
		object_array		fStack;
		PDFFontEncoding	*	fEncoding;
		
		uint32				fNumItems;
		uint32				fState;
		
		void				beginbfcharFn();
		void				endbfcharFn();
		void				beginbfrangeFn();
		void				endbfrangeFn();
		void				begincidcharFn();
		void				endcidcharFn();
		void				begincidrangeFn();
		void				endcidrangeFn();
		void				begincmapFn();
		void				endcmapFn();
		void				begincodespacerangeFn();
		void				endcodespacerangeFn();
		void				beginnotdefcharFn();
		void				endnotdefcharFn();
		void				beginnotdefrangeFn();
		void				endnotdefrangeFn();
		void				beginrearrangedfontFn();
		void				endrearrangedfontFn();
		void				beginusematrixFn();
		void				endusematrixFn();
		void				usecmapFn();
		void				usefontFn();
		
		void				get_num_items(int32 type);
		void				check_state(uint32 state, uint32 multiplier);
};

struct CMapOperators {
	CMapOperators();
	typedef const char * ccharp;
	ccharp
		beginbfchar,
		endbfchar,
		beginbfrange,
		endbfrange,
		begincidchar,
		endcidchar,
		begincidrange,
		endcidrange,
		begincmap,
		endcmap,
		begincodespacerange,
		endcodespacerange,
		beginnotdefchar,
		endnotdefchar,
		beginnotdefrange,
		endnotdefrange,
		beginrearrangedfont,
		endrearrangedfont,
		beginusematrix,
		endusematrix,
		usecmap,
		usefont,
		def;
};

#endif
