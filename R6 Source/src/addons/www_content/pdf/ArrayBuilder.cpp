#include "ArrayBuilder.h"
#include "PDFKeywords.h"


ArrayBuilder::ArrayBuilder()
	: ObjectSink()
{
}


ArrayBuilder::~ArrayBuilder()
{
}

status_t 
ArrayBuilder::UnknownKeyword(PDFObject *o)
{
	// don't need the keyword any longer
	o->Release();
	return ObjectSink::ERROR;
}

status_t 
ArrayBuilder::DispatchKeyword(PDFObject *o)
{
	status_t result = ObjectSink::OK;
	pdf_keys keyword = (pdf_keys)o->GetInt32();	// NOTE: overloaded cast!

	switch (keyword)
	{
	// [ - start array
	// << - start dict
		// push index of first element
		case PDF_startarray:
		case PDF_startdict:
			fIndicies.push_back(fStack.end() - fStack.begin());
			// don't need the keyword any longer
			o->Release();
			break;
	// ] - end array
		// make a new array, starting from the index on the top of the index stack
		// to the last object on the stack, then push the array on the stack
	// >> - end dict
		// make a new dict, starting from the index on the top of the index stack
		// to the last object on the stack, then push the dict on the stack
		case PDF_endarray:
		case PDF_enddict:
		{
			// duplicate the entries.  The new array will take our refs.
			object_array::iterator first = fStack.begin() + fIndicies.back();
			PDFObject *array = PDFObject::makeArray(first, fStack.end());
			fStack.erase(first, fStack.end());
			fStack.push_back(array);
			fIndicies.pop_back();
			if (keyword == PDF_enddict) array->PromoteToDictionary();
			// don't need the keyword any longer
			o->Release();
		} break;
		default:
			result = UnknownKeyword(o);
			break;
	}
	return result;
}

void 
ArrayBuilder::FlushStacks(void)
{
	for (object_array::iterator i = fStack.begin(); i != fStack.end(); i++)
		(*i)->Release();
	fStack.erase(fStack.begin(), fStack.end());
	fIndicies.erase(fIndicies.begin(), fIndicies.end());
}


#if 0
ssize_t 
ArrayBuilder::Write(BPrivate:: PDFObject *obj)
{
	status_t result = ObjectSink::OK;
	if (obj->IsKeyword()) result = DispatchKeyword(obj);
	else
	{
		// decrypt now
		if (obj->IsString() && fDecoder)
		{
			fDecoder->Encode(const_cast<uint8 *>(obj->Contents()), obj->Length());
			fDecoder->Reset();
		}
		// push item on stack
		fStack.push_back(obj);
	}
	return result;
}
#endif

