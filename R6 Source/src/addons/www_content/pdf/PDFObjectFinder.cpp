
#include "PDFObjectFinder.h"

using namespace BPrivate;

PDFObjectFinder::PDFObjectFinder(PDFObject *base) :
	fBase(NULL),
	fDoc(NULL)
{
	SetBase(base);
}


PDFObjectFinder::~PDFObjectFinder()
{
	SetBase(0);
}

status_t 
PDFObjectFinder::SetBase(PDFObject *base)
{
	if (fBase)
		fBase->Release();
	fBase = base;
	Initialize();

	return B_OK;
}

PDFObject *
PDFObjectFinder::Find(const char *name, bool inherited)
{
	// look for an entry named name
	// if we don't find such an entry, search in its parent
	PDFObject *entry = 0;
	for (size_t i = 0; i < fObjects.size(); i++)
	{
		entry = fObjects[i]->Find(name);
		if (entry) return entry->Resolve();
		if (!inherited) break;
	}
	return 0;
}

PDFObject *
PDFObjectFinder::FindResource(const char *type, const char *name)
{
	// look for a resource of type type named name
	PDFObject *res = 0;

	// find the resource dictionary in our lineage
	PDFObject *resdict = Find(PDFAtom.Resources, true);
	if (resdict->IsDictionary())
	{
		PDFObject *a_type = resdict->Find(type)->Resolve();
		if (a_type->IsDictionary())
		{
			// find the name in this dict
			res = a_type->Find(name)->Resolve();
		}
		else if (a_type->IsArray())
		{
			// return the array itself
			res = a_type;
			// pick up a reference for the caller
			res->Acquire();
		}
		// drop our copy of a_type
		a_type->Release();
	}
	// we don't need resdict, regardless of what it was
	resdict->Release();

	// return the resource
	return res;

}

void 
PDFObjectFinder::Initialize()
{
	// release any previous entries
	fObjects.Release();
	fObjects.resize(0);
	// store the reolved Page or Pages dictionary in the array
	// fPages[0] is the Page
	if (fBase) {
		fBase->Acquire(); // aquire so we can release it later and still have a copy
		fObjects.push_back(fBase);
	#if 1
		// subsequent indices are earlier Pages ancestors
		PDFObject *parent = fObjects.back()->Find(PDFAtom.Parent)->AsDictionary();
		while (parent)
		{
			fObjects.push_back(parent);
			parent = fObjects.back()->Find(PDFAtom.Parent)->AsDictionary();
		}
	#endif
	//	printf("fObjects.size() %lu\n", fObjects.size());
		// get the document
		PDFObject *obj = fBase->Find(PDFAtom.__document__);
		fDoc = (PDFDocument *)(((PDFOpaquePointer *)obj->GetOpaquePtr())->fPointer);
	}
}

