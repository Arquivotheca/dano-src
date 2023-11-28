#ifndef _PDF_DOC_H
#define _PDF_DOC_H

#include <stdio.h>
#include "Object2.h"
#include "Atomizer.h"
#include "MultiPositionIO.h"
#include "HashCache.h"
#include "PDFAlert.h"

class BPositionIO;

#define USE_OUTLINE 0

#if USE_OUTLINE > 0
class BOutline;
class BPDFOutlineItem;
#endif

namespace BPrivate {

struct xref_entry {
	xref_entry();
	fpos_t	offset;		// offset in PDF file of object definition
	//PDFObject *object;	// a pointer to the cached object
	uint16	generation;	// the object's generation number
	int16	in_use;		// use count if used, SHRT_MIN if on free list
};

struct xref_subtable {
	fpos_t	offset;		// offset in PDF file of xref entry for first object
	uint32	first;		// first object in this xref table (inclusive)
	uint32	last;		// last object in this xref table (inclusive)
	xref_subtable
			*next;		// next subtable in linked list of tables
};

typedef vector<xref_entry> xref_vector;

typedef vector<PDFObject *> dictionary_ptr_vector;

class Objectizer;

class PDFDocument : public HashCache {
public:
	PDFDocument(BPositionIO *source, PDFAlert &alert, bool ignoreLinearStatus = false);
	~PDFDocument();

bool		IsLinear(void) { return m_linear; };
PDFObject	*GetObject(uint32 object, uint16 generation);
void		SetObject(uint32 object, uint16 generation, PDFObject *obj);

uint32		PageCount(void);
const char *Title(void);

#if USE_OUTLINE > 0
status_t	GetOutline(uint32 type, BOutline *outline);
#endif

PDFObject *	GetNamedObject(const char *name_tree, PDFObject *name);
PDFObject * GetNamedDestination(PDFObject *name);
PDFObject *	GetPage(PDFObject *ref);
PDFObject *	GetPage(uint32 page);
int32		GetPageNumber(PDFObject *page);
BPositionIO *	Source() {return m_multiIO->GetClient();};

bool		IsEncrypted(void) { return m_encrypted; };
bool		SetPassword(const char *key, int key_length);

void		FlushXrefTable(void);

void		MakeObjectKey(uint32 object, uint16 generation, uchar *key_buffer);

void		ThrowAlert(status_t error) { m_alert.ThrowAlert(error); };

status_t	InitCheck() { return (m_trailer && m_catalog && m_pages) ? m_init : B_FILE_ERROR; };

private:
virtual void	DisposeEntry(void *value);
void		PopulateXrefEntry(uint32 object);
off_t		ParseXrefSection(Objectizer *src);
void		BuildXRefTable(void);

#if 0
void		ResolveOutlineDictionary(PDFObject *outline);
#endif

#if USE_OUTLINE > 0
void DictionaryToOutline(PDFObject *dict, BOutline *o, BPDFOutlineItem *parent);
#endif

BPositionIO	*m_source;
MultiPositionIOServer	*m_multiIO;
bool		m_linear;
bool		m_encrypted;
uint8		m_crypto_key[5];
xref_subtable
			*m_xref_tables;
xref_vector	m_xrefs;
//BAtomizer	*m_atomizer;
PDFObject	*m_trailer;
uint32		m_page_count;
PDFObject	*m_catalog;	// document catalog (aka m_trailer->/Root)
PDFObject	*m_pages;	// root of pages tree
PDFObject 	*m_title;
PDFObject	*m_dests;	// PDF 1.1 named destinations
PDFAlert	m_alert;
status_t	m_init;
};

}; // namespace BPrivate

using namespace BPrivate;

#endif
