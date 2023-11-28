#include "pdf_doc.h"
#include "Objectizer.h"
#include "PDFKeywords.h"
#include "IndirectBuilder.h"
#include "ObjectParser.h"
#include <Debug.h>

#include <limits.h>
#include <unistd.h>
#include <typeinfo>
#include <alloca.h>

#include "CachingPositionIO.h"
#include "MultiPositionIO.h"
#include "lex_maps.h"

#if USE_OUTLINE > 0
#include "Outline.h"
#include "PDFOutlineItem.h"
#endif

#include "md5.h"
#include "rc4.h"
#include "crypto.h"
#include <string.h>
#include <OS.h>

#if 0
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define xprintf(a) printf a
#define PRINTF_DEBUGGING 1
#else
#define xprintf(a)
#define PRINTF_DEBUGGING 0
#endif

using namespace BPrivate;

enum {
	B_PDF_DESTS = 0,
	B_PDF_OUTLINES,
	B_PDF_PAGES,
	B_PDF_THREADS,
	B_PDF_NAMES
} outline_type;

static uint8 password_padding[] = {
0x28, 0xbf, 0x4e, 0x5e, 0x4e, 0x75, 0x8a, 0x41, 0x64, 0x00, 0x4e, 0x56, 0xff, 0xfa, 0x01, 0x08,
0x2e, 0x2e, 0x00, 0xb6, 0xd0, 0x68, 0x3e, 0x80, 0x2f, 0x0c, 0xa9, 0xfe, 0x64, 0x53, 0x69, 0x7a
};

#define LEFT_SQUARE_STRING "["


xref_entry::xref_entry() :
	offset(-1), /* object(NULL), */ generation(0), in_use(SHRT_MIN)
{
}



PDFDocument::PDFDocument(BPositionIO *source, PDFAlert &alert, bool ignoreLinearStatus)
:	HashCache(1024),
	m_source(source),  m_linear(false), m_encrypted(false),
	m_xref_tables(0),
	m_trailer(PDFObject::makeDictionary()),
	m_page_count(0),
	m_catalog(0),
	m_pages(0),
	m_title(0),
	m_dests(0),
	m_alert(alert),
	m_init(B_NO_INIT)
{
#if 0
	m_multiIO = new MultiPositionIOServer(source);

	// size the cache appropriately
	while (contentLength < (1U << (cacheBits-1)))
		cacheBits--;

	m_cachingIO = new CachingPositionIO(m_multiIO->GetClient(), cacheBits, contentLength);
	m_cachingIO->FillCache(0, len, buffer);
	
	m_tokenizer = new Tokenizer(m_cachingIO);
#endif

	m_multiIO = new MultiPositionIOServer(source);

	// mark password uncreated
	//memcpy(m_crypto_key, "UNSET", sizeof(m_crypto_key));
	if (!ignoreLinearStatus) {
		// check for a linearized document
	}

	// will set m_init on failure
	BuildXRefTable();

	if (m_init == B_NO_INIT)
	{
#ifndef NDEBUG
		printf("trailer: "); m_trailer->PrintToStream(); printf("\n");
#endif
		if (m_trailer->Find(PDFAtom.Encrypt)) m_encrypted = true;
		xprintf(("Document %s encrypted\n", m_encrypted ? "is" : "is not"));
	
		// each page is represented by a dictionary
		if ((m_catalog = m_trailer->Find(PDFAtom.Root)->AsDictionary()))
		{
			m_pages = m_catalog->Find(PDFAtom.Pages)->AsDictionary();
			if (m_pages)
			{
				PDFObject *info = m_trailer->Find(PDFAtom.Info)->AsDictionary();
				if (info)
				{
					m_title = info->Find(PDFAtom.Title)->AsString();
					info->Release();
				}
				m_init = B_OK;
			}
			else m_init = B_FILE_ERROR;
		}
		else m_init = B_FILE_ERROR;
	}
	if (m_init != B_OK) ThrowAlert(m_init);
}

PDFDocument::~PDFDocument()
{
	if (m_title)
		m_title->Release();
	if (m_catalog) 
		m_catalog->Release();
	if (m_pages)
		m_pages->Release();
	if (m_trailer)
		m_trailer->Release();
	if (m_dests)
		m_dests->Release();
	FlushXrefTable();
	MakeEmpty();	// HashCache, just in case
	while (m_xref_tables)
	{
		xref_subtable *xs = m_xref_tables;
		m_xref_tables = m_xref_tables->next;
		delete xs;
	}
	delete m_multiIO;
}

const char *
PDFDocument::Title(void)
{
	// works even if m_title contains NULL
	return m_title->GetCharPtr();
}

static bool IsValidUserPassword(const uchar *digest, const PDFObject *U)
{
	RC4_KEY key;
	uint8 u[32];

	// set up to decode U
	memcpy(u, U->Contents(), 32);
	// use the digest as the RC4 key
	RC4_set_key(&key, 5, (uchar *)digest);
	// decrypt
	RC4(&key, 32, u, u);
	// validate
	if (memcmp(u, password_padding, 32) == 0) return true;
	xprintf(("IsValidUserPassword() is false\n"));
	return false;
}

static bool IsValidOwnerPassword(const MD5_CTX *ctx, const PDFObject *O, const PDFObject *U)
{
	return false;
}

static void MakePaddedKey(uint8 *padded_password, const char *key, int key_length)
{
	memcpy(padded_password, key, key_length < 32 ? key_length : 32);
	if (key_length < 32) memcpy(padded_password+key_length, password_padding, 32 - key_length);
#if PRINTF_DEBUGGING
	xprintf(("padded password:"));
	for (int i = 0; i < 32; i++)
		xprintf((" %.2x", padded_password[i]));
	xprintf(("\n"));
#endif
}

bool 
PDFDocument::SetPassword(const char *key, int key_length)
{
	// don't bother if not encrypted
	if (!m_encrypted) return true;

	bool result = false;
	PDFObject *crypt_dict = 0;
	PDFObject *Filter = 0;
	PDFObject *ID = 0;
	PDFObject *IDstring = 0, *O = 0, *U = 0;
	PDFObject *P = 0, *R = 0, *V = 0;
	MD5_CTX context, owner_context;
	uchar padded_password[32];
	uint32 p_int;
	uchar p_key[4];
	uchar digest[MD5_DIGEST_LENGTH];

	// temporarily mark the file unencrypted
	m_encrypted = false;

	xprintf(("key: %s, length: %d\n", key, key_length));
	// look for the Encrypt dictionary in the trailer dictionary
	if (!(crypt_dict = (m_trailer->Find(PDFAtom.Encrypt))->AsDictionary()))
		goto missing_crypt_dict;

	// find the document ID or die trying	
	if (!(ID = (m_trailer->Find(PDFAtom.ID))->AsArray()) ||
		!(IDstring = ((*(ID->Array()))[0])->AsString())
		)
		goto missing_ID;

	// find a /Filter /Standard, or die trying
	if (!(Filter = crypt_dict->Find(PDFAtom.Filter)->AsName()) ||
		((const char *)Filter == PDFObject::Atom("Standard"))
		)
		goto missing_Filter;

	// get the /R key or die trying
	if (!(R = crypt_dict->Find(PDFObject::Atom("R"))->AsNumber()) ||
		(R->GetInt32() != 2) // we only support /Standard, Revision 2
		)
		goto missing_R;

	// get the /V key or die trying
	if (!(V = crypt_dict->Find(PDFObject::Atom("V"))->AsNumber()) ||
		(V->GetInt32() != 1) // we only support Version 1
		)
		goto missing_V;
	
	// get the encrypted /O key or die trying
	if (!(O = crypt_dict->Find(PDFObject::Atom("O"))->AsString()))
		goto missing_O;

	// get the encrypted /U key or die trying
	if (!(U = crypt_dict->Find(PDFObject::Atom("U"))->AsString()))
		goto missing_U;

	// get the encrypted /P key or die trying
	if (!(P = crypt_dict->Find(PDFObject::Atom("P"))->AsNumber()))
		goto missing_P;

	MD5_Init(&context);
	// Algorithm 6.7, step 1
	MakePaddedKey(padded_password, key, key_length);
	// Algorith 6.7, step 2
	MD5_Update(&context, (uchar *)padded_password, 32);
	// Algorith 6.7, step 3
	MD5_Update(&context, (uchar *)O->Contents(), O->Length());
	// save the context for Owner key testing
	owner_context = context;
	// Algorithm 6.7, step 4
	p_int = P->GetInt32();
#if 0
	/* NOTE that though the spec says that's what the bits below should be set to,
	one shouldn't force the bits this way if they're not set/clears as indicated */
	/* bits 7-32 (ie: 6-31) are reserved and set to 1 */
	p_int |= 0xffffffc0;
	/* bits 1-2 (ie: 0-1) are reserved and cleared to 0 */
	p_int &= 0xfffffffc;
#endif
	p_key[0] = (p_int >>  0) & 0xff;
	p_key[1] = (p_int >>  8) & 0xff;
	p_key[2] = (p_int >> 16) & 0xff;
	p_key[3] = (p_int >> 24) & 0xff;
	MD5_Update(&context, p_key, 4);
	// Algorithm 6.7, step 5
	MD5_Update(&context, (uchar *)IDstring->Contents(), IDstring->Length());
	// Algorithm 6.7, step 6
	MD5_Final(digest, &context);
	if (!IsValidUserPassword(digest, U))
	{
		RC4_KEY key;
		uint8 o[32];
		MD5_Final(digest, &owner_context);
#if PRINTF_DEBUGGING
		xprintf(("owner digest bytes: "));
		for (int j = 0; j < MD5_DIGEST_LENGTH; j++)
			xprintf(("%.2x", digest[j]));
		xprintf(("\n"));
#endif
		memcpy(o, O->Contents(), 32);
#if PRINTF_DEBUGGING
		xprintf(("o before:"));
		for (int i = 0; i < 32; i++)
			xprintf((" %.2x", o[i]));
		xprintf(("\n"));
#endif
		// use the digest as the RC4 key
		RC4_set_key(&key, 5, (uchar *)digest);
		// decrypt
		RC4(&key, 32, o, o);

#if PRINTF_DEBUGGING
		xprintf(("o  after:"));
		for (int i = 0; i < 32; i++)
			xprintf((" %.2x", o[i]));
		xprintf(("\n"));
#endif

		MD5_Init(&context);
		// Algorith 6.7, step 2
		MD5_Update(&context, (uchar *)padded_password, 32);
		// Algorith 6.7, step 3
		MD5_Update(&context, o, 32);
		// Algorithm 6.7, step 4
		MD5_Update(&context, (uchar *)p_key, 4);
		// Algorithm 6.7, step 5
		MD5_Update(&context, (uchar *)IDstring->Contents(), IDstring->Length());
		// Algorithm 6.7, step 6
		MD5_Final(digest, &context);
		// use the digest as the RC4 key
		RC4_set_key(&key, 5, (uchar *)digest);
		memcpy(o, U->Contents(), 32);
		// decrypt
		RC4(&key, 32, o, o);
#if PRINTF_DEBUGGING
		xprintf(("u after2:"));
		for (int i = 0; i < 32; i++)
			xprintf((" %.2x", o[i]));
		xprintf(("\n"));
#endif

		//if (!IsValidOwnerPassword(digest, O, U))
		goto bad_password;
	}
	result = true;
	// save the keyword
	memcpy(m_crypto_key, digest, 5);

bad_password:
missing_P:
missing_U:
missing_O:
missing_V:
missing_R:
missing_Filter:
missing_ID:
missing_crypt_dict:
	P->Release();
	U->Release();
	O->Release();
	V->Release();
	R->Release();
	Filter->Release();
	ID->Release();
	IDstring->Release();
	crypt_dict->Release();
	// restore the encryption flag
	m_encrypted = true;
	// all done
	return result;
}

#if USE_OUTLINE > 0
static uint32 indexNum = 0;

status_t 
PDFDocument::GetOutline(uint32 type, BOutline *outline)
{
	if (type != B_PDF_OUTLINES) return B_ERROR;
	if (!outline) return B_BAD_VALUE;
	indexNum = 0;	
	if (PDFObject *Root = m_trailer->Find(PDFAtom.Root)->AsDictionary())
	{
		if (PDFObject *dict = Root->Find(PDFAtom.Outlines)->AsDictionary())
		{
			DictionaryToOutline(dict, outline, NULL);
			dict->Release();
			Root->Release();
			//FlushXrefTable();
			return B_OK;
		}
		Root->Release();
	}
	return B_ERROR;
}

void
PDFDocument::DictionaryToOutline(PDFObject *dict, BOutline *o, BPDFOutlineItem *p)
{
	BPDFOutlineItem *parent = NULL;
	
	if (PDFObject *s = dict->Find(PDFAtom.Title)->AsString())
	{
		int32 len = s->Length();
		char *buf = (char *) alloca(len + 1);
		memcpy(buf, s->Contents(), len);
		buf[len] = '\0';
//		printf("PDFObject->Contents(): %s\n", buf);
		parent = new BPDFOutlineItem(indexNum++, buf, 0, p);
		o->Graft(parent);
		s->Release();
	}
	
	PDFObject *kid_dict;
	if ((kid_dict = dict->Find(PDFAtom.First)->AsDictionary()))
	{
		DictionaryToOutline(kid_dict, o, parent);
		kid_dict->Release();
	}

	if ((kid_dict = dict->Find(PDFAtom.Next)->AsDictionary()))
	{
		DictionaryToOutline(kid_dict, o, p);
		kid_dict->Release();
	}
}
#endif

uint32 
PDFDocument::PageCount(void)
{
	if (!m_page_count)
	{
		PDFObject *pagenum = m_pages->Find(PDFAtom.Count)->AsNumber();
		if (pagenum)
		{
			m_page_count = pagenum->GetInt32();
			pagenum->Release();
		}
	}
	return m_page_count;
}

PDFObject *
PDFDocument::GetNamedObject(const char *name_tree, PDFObject *name_obj)
{
	const char *name = name_obj->GetCharPtr();
	uint32 name_length = name_obj->Length();
	PDFObject *parent;
	PDFObject *kids;
	PDFObject *result = 0;
	// pick name_tree from /Root /Names
	PDFObject *child = m_catalog->Find(PDFAtom.Names)->AsDictionary();
	if (child)
	{
	// search for the partiular name tree
	parent = child->Find(name_tree)->AsDictionary();
	// drop our reference to the /Names dict
	child->Release();
	child = 0;
	// bail out if no dict by that name
	if (parent)
	{
	kids = parent->Find(PDFAtom.Kids)->AsArray();

	// search for the proper leaf
	while (kids)
	{
		// binary search on kids to find the branch/leaf with the desired name
		object_array *oa = kids->Array();
		int32 top = 0;
		int32 bottom = oa->size() - 1;
		while (top <= bottom)
		{
			int32 middle = (bottom + top) / 2;
			child = (*oa)[middle]->AsDictionary();
			ASSERT(child != 0);
			PDFObject *limits = child->Find(PDFAtom.Limits)->AsArray();
			ASSERT(limits != 0);
			// just in case
			limits->ResolveArrayOrDictionary();
			PDFObject *first = (*(limits->Array()))[0];
			PDFObject *second = (*(limits->Array()))[1];
			uint32 length = first->Length();
			if (length > name_length) length = name_length;
			if (strncmp(name, first->GetCharPtr(), length) < 0)
			{
				// name comes before first
				bottom = middle - 1;
			}
			else
			{
				length = second->Length();
				if (length > name_length) length = name_length;
				if (strncmp(name, second->GetCharPtr(), length) > 0)
				{
					// name comes after second
					top = middle + 1;
				}
				else
				{
					// name falls within the keys
					limits->Release();
					break;
				}
			}
			// don't need this child any longer
			child->Release();
			child = 0;
			limits->Release();
			limits = 0;
		}
		// don't need the current kids any longer
		kids->Release();
		// or the parent
		parent->Release();
		parent = child;
		child = 0;
		if (parent) kids = parent->Find(PDFAtom.Kids)->AsArray();
		else kids = 0;
	}
	// search within the leaf node
	if (parent)
	{
		// binary search of name/value pairs for desired name
		PDFObject *child = parent->Find(PDFAtom.Names)->AsArray();
		object_array *oa = child->Array();
		int32 top = 0;
		int32 bottom = oa->size() / 2 - 1; // index of last key, not value
		int32 middle = 0;
		bool match = false;
		while (top <= bottom)
		{
			middle = (bottom + top) / 2;
			PDFObject *string = (*oa)[middle * 2]->AsString();
			uint32 length = string->Length();
			if (length > name_length) length = name_length;
			int32 cmp = strncmp(name, string->GetCharPtr(), length);
			if ((cmp < 0) || ((cmp == 0) && (string->Length() > name_length)))
			{
				// name comes before
				bottom = middle - 1;
			}
			else if ((cmp > 0) || ((cmp == 0) && (string->Length() < name_length)))
			{
				// name comes after
				top = middle + 1;
			}
			else
			{
				// match!
				string->Release();
				match = true;
				break;
			}
			string->Release();
		}
		if (match)
		{
			result = (*oa)[middle * 2 + 1];
			result->Acquire();
		}
		child->Release();
		parent->Release();
	}

	} // if parent
	} // if child
	return result;
}

PDFObject *
PDFDocument::GetNamedDestination(PDFObject *name)
{
	if (!m_dests)
	{
		m_dests = m_catalog->Find(PDFAtom.Dests)->AsDictionary();
		if (!m_dests) return 0;
#ifndef NDEBUG
		m_dests->PrintToStream(4);
#endif
	}
	return m_dests->Find(name)->Resolve();
}

PDFObject *
PDFDocument::GetPage(PDFObject *ref)
{
	PDFObject *page = ref->AsDictionary();
	if (page)
	{
		// validate
		PDFObject *type = page->Find(PDFAtom.Type);
		if (type->GetCharPtr() == PDFAtom.Page)
		{
			// add doc info
			page->Assign(PDFObject::makeName(PDFAtom.__document__), PDFObject::makeOpaque(new PDFOpaquePointer(this)));
		}
		else
		{
			page->Release();
			page = 0;
		}
	}
	return page;
}

PDFObject *
PDFDocument::GetPage(uint32 page)
{
	// find the page'th page, starting from m_trailer->/Root
	// valid page range: 1 thru #pages
	PDFObject *parent = m_pages;
	PDFObject *child = 0;
	PDFObject *count_obj = 0; //parent->Find(PDFAtom.Count)->AsNumber();
	uint32 count = 0; //(uint32)*count_obj;
	PDFObject *kids = parent->Find(PDFAtom.Kids)->AsArray();
	uint32 first = 0;
	uint32 index = 0;

	// acuire the parent so we can release it later
	parent->Acquire();

	// bail if page out of range
	if (!page || (page > PageCount())) return 0;

	// until we're done
	while (true)	// or perhaps while (first == page)
	{
#if DEBUG > 2
		printf("Kids "); kids->PrintToStream(2); printf("\n");
#endif
		do
		{
			// bump start of group
			first += count;
			// release the old child
			child->Release();
			// advance to next child in this Pages dictionary
			PDFObject *element = ((*(kids->Array()))[index++]);
			ASSERT(element);
#if DEBUG > 2
			printf("element %lu ", index - 1); element->PrintToStream(1); printf("\n");
#endif
			child = element->AsDictionary();
			ASSERT(child);
			ASSERT(child->IsDictionary());
			// get count of number of children in this dictionary
			count_obj = child->Find(PDFAtom.Count)->AsNumber();
			count = count_obj ? count_obj->GetInt32() : 1; // use count, or one for Page dictionary
			// release the old count object
			count_obj->Release();
		}
		while ((first + count) < page);
		
		// make this child the new parent
		parent->Release();
		ASSERT(child);
		parent = child;
		child = 0;
		// don't need these kids any longer
		kids->Release();
	
		// if this is-a Pages, descend
		PDFObject *type = parent->Find(PDFAtom.Type);
		if (type->IsName() && (type->GetCharPtr() == PDFAtom.Pages))
		{
			// get the new kids array
			kids = parent->Find(PDFAtom.Kids)->AsArray();
			// restart the index
			index = 0;
			// restart counting, too
			count = 0;
			// continue loop
			continue;
		}
		// we must have found the desired page
		ASSERT(first+count == page);
		ASSERT(type->GetCharPtr() == PDFAtom.Page);
		break;
	}

	// remind them where they got it
	if (parent)
		parent->Assign(PDFObject::makeName(PDFAtom.__document__), PDFObject::makeOpaque(new PDFOpaquePointer(this)));

	// return the page.  The caller takes possesion of the reference.
	return parent;
}

int32
PDFDocument::GetPageNumber(PDFObject *target)
{
	PDFObject *parent = target->Find(PDFAtom.Parent)->AsDictionary();
	int32 pagenum = 1;
	
	target->Acquire();
	
	do
	{
		int32 count = 0;
		int32 index = 0;
		PDFObject *kids = parent->Find(PDFAtom.Kids)->AsArray();
		PDFObject *child;
		PDFObject *count_obj;
		// count all the items before this one
		do
		{
			pagenum += count;
			// advance to next child in this Pages dictionary
			child = ((*(kids->Array()))[index++])->AsDictionary();
			// get count of number of children in this dictionary
			count_obj = child->Find(PDFAtom.Count)->AsNumber();
			count = count_obj ? count_obj->GetInt32() : 1; // use count, or one for Page dictionary
			// release the old count object
			count_obj->Release();
			// release the child
			child->Release();
		}
		while (child != target);
		kids->Release();
		// release the target
		target->Release();
		// walk up the parent chain
		target = parent;
		parent = target->Find(PDFAtom.Parent)->AsDictionary();		
	}
	while (parent);
	target->Release();
	return pagenum;
}

void 
PDFDocument::MakeObjectKey(uint32 object, uint16 generation, uchar *key_buffer)
{
	// first 5 bytes of key source
	memcpy(key_buffer, m_crypto_key, 5);
	// last 5 bytes of key source
	key_buffer[5] = (object >>  0) & 0xff;
	key_buffer[6] = (object >>  8) & 0xff;
	key_buffer[7] = (object >> 16) & 0xff;
	key_buffer[8] = (generation >>  0) & 0xff;
	key_buffer[9] = (generation >>  8) & 0xff;
	// MD5 hash the key
	MD5Hash hash;
	hash.Update(key_buffer, 10);
	hash.Final();
	// copy the key to the caller
	memcpy(key_buffer, hash.Digest(), 10);
}

const uint8 *
strnstrn(const uint8 *buf, size_t buf_len, const uint8 *key, size_t key_len)
{
	const uint8 *p = 0;
	
	while (buf_len >= key_len)
	{
		if (memcmp(buf, key, key_len) == 0)
		{
			p = buf;
			break;
		}
		// advance
		buf++;
		buf_len--;
	}
	return p;
}

const uint8 *
strnrstrn(const uint8 *buf, size_t buf_len, const uint8 *key, size_t key_len)
{
	const uint8 *p = 0;
	const uint8 *buf_start = buf - 1;
	buf += (buf_len - key_len);

	while (buf != buf_start)
	{
		if (memcmp(buf, key, key_len) == 0)
		{
			p = buf;
			break;
		}
		// advance
		buf--;
	}
	return p;
}

void 
PDFDocument::DisposeEntry(void *value)
{
	// release the entry
	PDFObject *obj = (PDFObject *)value;
	//ASSERT(obj && (obj->GetRefCount() == 1));
#ifndef NDEBUG
	if (obj->GetRefCount() > 1)
	{
		printf("DisposeEntry() dropping in-use resource:"); obj->PrintToStream(3); printf("\n");
	}
#endif
	obj->Release();
}

PDFObject *
PDFDocument::GetObject(uint32 object, uint16 generation)
{
	PDFObject *o = 0;
#if DEBUG > 2
	printf("m_xrefs.size(): %lu\n", m_xrefs.size());
#endif
	if (object > m_xrefs.size())
	{
		// not found, return a null object
		//printf("GetObject(%lu) ran past end %lu\n", object, m_xrefs.size());
		o = PDFObject::makeNULL();
	}
	else
	{
		xref_entry &xe = m_xrefs[object]; //(*i).second;

		// have we seen this object before
		o = (PDFObject *)Find(object);
#if 0
		if (xe.object)
		{
			// use the cached object
			o = xe.object;
			//printf("GetObject(%lu) using cached object %p\n", object, o);
		}
		else
#else
		if (!o)
#endif
		{
			// if we haven't seen this object yet, load it
			if (xe.offset == -1) PopulateXrefEntry(object);
			//if (object == 19) DEBUGGER("found object 19");
			if ((xe.generation == generation) && (xe.in_use != SHRT_MIN))
			{
				IndirectBuilder *ib = new IndirectBuilder(this);
				ObjectParser *op = new ObjectParser(ib);
				static const ssize_t kMaxBufferSize = 1024;
				uint8 *buffer = new uint8[kMaxBufferSize];
				BPositionIO *src = Source();
				ssize_t readResult, writeResult, totalWrote = 0;
				// seek to start of object
				src->Seek(xe.offset, SEEK_SET);
				do
				{
					// fill the buffer
					readResult = src->Read(buffer, kMaxBufferSize);
					if (readResult <= 0) break;
					// write the buffer
					writeResult = op->Write(buffer, readResult, false);
					if (writeResult >= 0) totalWrote += writeResult;
				}
				while (writeResult == readResult);
				// get the object we just made
				o = (PDFObject *)Find(object);
#if DEBUG > 2
				printf("GetObject(%lu) got ", object); o->PrintToStream(1); printf("\n");
#endif
				ASSERT(o != 0);
				// a stream?
				if (o && o->IsDictionary() && o->Find(PDFAtom.__stream_off_t__))
				{
					// skip past \n or \r\n after stream keyword
					off_t position = xe.offset + totalWrote;
					// Except if the first filter converts from ASCII*
					//  This fixes files that don't follow the \n or \r\n spec and just use \r
					PDFObject *filter = o->Find(PDFAtom.Filter)->Resolve();
					if (filter->IsArray())
					{
						object_array *oa = filter->Array();
						PDFObject *tmp = (*oa)[0]->Resolve();
						filter->Release();
						filter = tmp;
					}
					src->ReadAt(position, buffer, 2);
					//ASSERT(object != 31);
					if (filter)
					{
						// an ascii filter?
						if ((filter->GetCharPtr() == PDFAtom.ASCII85Decode) || (filter->GetCharPtr() == PDFAtom.ASCIIHexDecode))
						{
							// consume one or two bytes of \r and \n
							position += (*(buffer+1) == '\n') ? 2 : 1;
						}
						// not an ascii filter, so follow the spec
						else position += (*buffer == '\r') ? 2 : 1;
					}
					// no filter, so follow the spec
					else position += (*(buffer+1) == '\n') ? 2 : 1;

					filter->Release();
					// store the stream offset in the dictionary
					o->Assign(PDFObject::makeName(PDFAtom.__stream_off_t__), PDFObject::makeNumber(position));
					// encryption key, too
					if (IsEncrypted())
					{
						uchar key_buff[10];
						MakeObjectKey(object, xe.generation, key_buff);
						o->Assign(PDFObject::makeName(PDFAtom.__stream_key__), PDFObject::makeString(sizeof(key_buff), key_buff));
					}
					// save document pointer in stream for easy access
					o->Assign(PDFObject::makeName(PDFAtom.__document__), PDFObject::makeOpaque(new PDFOpaquePointer(this)));
				}
				//printf("GetObject made %lu\n", object); o->PrintToStream(3); printf("\n");
				// get rid of locals
				delete src;
				delete [] buffer;
				delete op;
			}
			else
			{
				// deleted object, return null
				o = PDFObject::makeNULL();
			}
		}
		if (o) o->Acquire();
	}
	return o;
}

void 
PDFDocument::SetObject(uint32 object, uint16 generation, PDFObject *obj)
{
	//printf("SetObject(%lu)\n", object);
	// replace whatever (if anything) we already have cached for object/generation
	// with the new object
	xref_entry &xe = m_xrefs[object]; //(*i).second;
	// save new object
	PDFObject *old = (PDFObject *)Intern(object, obj);
	xe.generation = generation;
	// throw away existing object
	if (old)
	{
#if !defined(NDEBUG)
		DEBUGGER("SetObject() replacing old entry");
		//printf("SetObject(%lu) replacing ", object); old->PrintToStream(1); printf("\n");
#endif
		old->Release();
	}
	// link in the back pointer
	if (obj) obj->SetXrefEntry(object);
}


void 
PDFDocument::PopulateXrefEntry(uint32 object)
{
	BPositionIO *src = Source();
	uint8 buffer[20];
	// find entry in table
	xref_subtable *subtable = m_xref_tables;
	while (subtable)
	{
		// break on match
		if ((object >= subtable->first) && (object <= subtable->last))
		{
			fpos_t offset;
			uint16 generation;
			uint8 *buf = buffer;
			// seek to proper location
			src->Seek(subtable->offset + (20 * (object - subtable->first)), SEEK_SET);
			// read the entry
			src->Read(buffer, 20);
			
			offset = 0;
			while ((*buf >= '0') && (*buf <= '9'))
			{
				offset *= 10;
				offset += *buf - '0';
				buf++;
			}
			buf++;
			generation = 0;
			while ((*buf >= '0') && (*buf <= '9'))
			{
				generation *= 10;
				generation += *buf - '0';
				buf++;
			}
			buf++;
			xref_entry &xe = m_xrefs[object];
			xe.generation = generation;
			xe.offset = offset;
			xe.in_use = (*buf == 'n' ? 0 : SHRT_MIN);
#if DEBUG > 2
			printf("object %lu, offset %Ld\n", object, offset);
#endif
			// always nill
			// xe.object = 0;
			//printf("Made entry for %lu, offset %Ld, generation %hu\n", object, offset, generation);
			break;
		}
		//printf("searching in next subtable\n");
		// advance to next entry
		subtable = subtable->next;
	}
	delete src;
}

off_t
PDFDocument::ParseXrefSection(Objectizer *src)
{
	uint32 first, count;
	xref_subtable *tail = m_xref_tables;
	xref_subtable *subtable = 0;
	PDFObject *o;
	off_t prev = 0;

	// bail if we can't find the "xref" keyword
	o = src->GetObject();
	//printf("Expecting xref keyword: "); o->PrintToStream(); printf("\n");
	if (!o->IsKeyword() || (o->GetInt32() != PDF_xref))
	{
		m_init = B_FILE_ERROR;
		goto exit0;
	}

	// don't need this keyword any longer
	o->Release();
	
	// grab the next object
	o = src->GetObject();

	// walk to the tail of the xref_subtable list
	while (tail && tail->next) tail = tail->next;
#if 0
	printf("tail: %p\n", tail);
	printf("m_xref_tables: %p\n", m_xref_tables);
#endif

	// walk to the tail of the xref_subtable list
	while (tail && tail->next) tail = tail->next;
#if 0
	printf("tail: %p\n", tail);
	printf("m_xref_tables: %p\n", m_xref_tables);
#endif

	// at this point, we've just parsed the 'xref' keyword and following EOL marker
	// what follows next should either be a pair of numbers indicating start object and count
	// or the 'trailer' keyword indicating the end of the xref section

	while (o->IsNumber()) {
		// figure out how big the sub table is
		first = (uint32)o->GetInt32(); o->Release();
		o = src->GetObject();
		count = (uint32)o->GetInt32(); o->Release();

		{
			uint8 aByte = ' ';
			// seek to current position to eliminate any buffering
			src->Seek(src->Position(), SEEK_SET);
			// skip to start of table
			while (InMap(whitemap, aByte) != 0)
				src->Read(&aByte, 1);
		}
		// make sure the xref vector is large enough
		if (m_xrefs.size() < (first + count))
			m_xrefs.resize(first + count);

		// make a new xref_subtable item
		subtable = new xref_subtable;
		if (subtable)
		{
			// link in the new node
			if (tail) tail->next = subtable;
			// keep track of the head of the list
			if (!m_xref_tables) m_xref_tables = subtable;
			// populate the node
			subtable->offset = src->Position();
			subtable->first = first;
			subtable->last = first + count - 1;
			subtable->next = 0;
			//printf("subtable: offset %Ld, first %ld, last %ld\n", subtable->offset, first, subtable->last);
			// make the new node the tail of the list
			tail = subtable;
		}
		else
		{
			debugger("new failed for xref_subtable entry");
			m_init = B_NO_MEMORY;
			return 0;
		}

		// seek past the table contents (20 bytes per entry), less one for overstepping earlier
		src->Seek(tail->offset + (count * 20) - 1, SEEK_SET);

		// grab the next token
		o = src->GetObject();
	}

	// bail out if we didn't find the required 'trailer' keyword
	if (!o->IsKeyword() || (o->GetInt32() != PDF_trailer))
	{
		m_init = B_FILE_ERROR;
		goto exit0;
	}
	o->Release();

	// ignore stuff until we see the required '<<' start of array marker
	o = src->GetObject();
	if (!o->IsDictionary())
	{
		m_init = B_FILE_ERROR;
		goto exit0;
	}
	// non-zero and invalid offset
	prev = 1;
	// save the /Prev key, if present
	if (PDFObject *n = o->Find(PDFAtom.Prev)->AsNumber())
	{
		prev = n->GetInt64();
		n->Release();
		o->Erase(PDFAtom.Prev);
	}
	// merge the contents of the current trailer dictionary with this one
	m_trailer->Merge(o);
	
exit0:
	// release the object
	o->Release();
	// return the offset of the next table
	return prev;
}

void
PDFDocument::BuildXRefTable(void)
{
	const uint8 *s;
	uint8 *buf = new uint8[2048];
	Objectizer *src = new Objectizer(this);
	off_t prev = 0;
	
	// get 2K worth of data from the end of the file
	if (src->Seek(-2048, SEEK_END) < 0) src->Seek(0, SEEK_SET);
	ssize_t bytesRead = src->Read(buf, 2048);
	if (bytesRead <= 0)
	{
		m_init = B_IO_ERROR;
		goto exit0;
	}
	// search for the last occurance of %%EOF
	s = strnrstrn(buf, bytesRead, (uint8*)"%%EOF", 5);
	if (!s)
	{
		m_init = B_FILE_ERROR;
		goto exit0;
	}
	// back up 26 bytes and look for "startxref"
	// 26 == strlen("startxref") + ten_digits_in_file_position + some_bytes_for_line_breaks
	if (s - buf < 26)
	{
		m_init = B_FILE_ERROR;
		goto exit0;
	}

	s = strnrstrn(s - 26, 26, (uint8*)"startxref", 9);
	// after startxref, throw away whitespace
	s += 9;
	while (InMap(whitemap, *s)) s++;
	// parse the number
	while ((*s >= '0') && (*s <= '9'))
	{
		prev *= 10;
		prev += *s - '0';
		s++;
	}
	
	do {
		// seek to the start of the next xref table
		src->Seek(prev, SEEK_SET);
		// parse the table, retrieving next offset
		prev = ParseXrefSection(src);
	} while (prev > 1);

exit0:
	// drop the buffer
	delete [] buf;
	// and the source IO
	delete src;
	// prev == 1 is OK
	//return prev ? B_OK : B_ERROR;
}

void 
PDFDocument::FlushXrefTable(void)
{
#if 0
	for (uint32 i = 0; i < m_xrefs.size(); i++)
	{
		if (PDFObject *o = m_xrefs[i].object)
		{
			if (o->GetRefCount() == 1)
			{
#if 0
				printf("%ld %u: ", i, m_xrefs[i].generation);
				o->PrintToStream(2);
#endif
#if 1
				o->SetXrefEntry(0); // ???
				o->Release();
				m_xrefs[i].object = 0;
#endif
			}
#if 1
			else printf("object %ld (%d)\n", i, o->GetRefCount());
#endif
		}
	}
#endif
}

#if 0
void
PDFDocument::ResolveOutlineDictionary(PDFObject *Outline)
{
	PDFObject *dict;
	// depth first
	if ((dict = Outline->Find(PDFAtom.First)->AsDictionary()))
	{
		ResolveOutlineDictionary(dict);
		dict->Release();
	}
	// siblings next
	if ((dict = Outline->Find(PDFAtom.Next)->AsDictionary()))
	{
		ResolveOutlineDictionary(dict);
		dict->Release();
	}
}

status_t 
PDFDocument::BuildTrees(void)
{

	status_t result = B_ERROR;
	PDFObject *Root = 0;
	PDFObject *Outline = 0;
	PDFObject *Pages = 0;
	
	// each page is represented by a dictionary
	if (!(Root = m_trailer->Find(PDFAtom.Root)->AsDictionary())) goto error0;

	if ((Pages = Root->Find(PDFAtom.Pages)->AsDictionary()))
	{
		// printf("Found Pages dictionary in Root\n");
		PDFObject *Count;
		if ((Count = Pages->Find(PDFAtom.Count)->AsNumber()))
		{
			m_pages = new object_array((int32)*Count);
			ResolvePagesDictionary(Pages);
			Count->Release();
		}
	}
#if 0
	if ((Outline = Root->Find(PDFAtom.Outlines)->AsDictionary()))
	{
		printf("Found Outline dictionary in Root\n");
		ResolveOutlineDictionary(Outline);
	}
#endif
	// printf("Found %ld pages\n", m_page_count);
	//printf("Root directory: ");
	//Root->PrintToStream(0);
error0:
	// Release() is OK to call on NULL pointers!
	Pages->Release();
	Outline->Release();
	Root->Release();
//	printf("leaving BuildTrees()\n");
	return result;
}
#endif

#define TESTING 0
#if TESTING > 0
#include <File.h>
#include <string.h>

#include <Application.h>
#include <Window.h>

int main(int argc, char **argv)
{
	BFile pdf_file(argv[1], B_READ_ONLY);
	status_t result = pdf_file.InitCheck();
	if (result != B_OK) {
		printf("pdf_file.InitCheck() failed with reason: 0x%08lx, %s\n", result, strerror(result));
		return -1;
	}
	PDFDocument doc(&pdf_file);
//	printf("pages found: %ld\n", doc.PageCount());
	
}
#endif
