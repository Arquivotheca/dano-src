/*	$Id: DDwarfType.h,v 1.2 1999/03/09 09:32:28 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/02/99 15:11:29
*/

#ifndef DDWARFTYPE_H
#define DDWARFTYPE_H

#include "DDwarf2.h"
#include "DType.h"
#include <map>

class DDwarfType : public DType
{
  public:
	DDwarfType (const string& name, const dwarf::entry& entry);
	DDwarfType (const dwarf::entry& entry);
	DDwarfType ();

	DDwarfType(const DDwarfType&);
	DDwarfType& DDwarfType::operator=(const DDwarfType&);

	virtual void GetName(string& outName) const;

	virtual bool IsStruct () const;
	virtual bool IsUnion () const;
	virtual bool IsEnumeration () const;
	virtual bool IsArray () const;
//	virtual bool IsTypedef () const;
	virtual bool IsPointer () const;
	virtual bool IsReference () const;
	virtual bool IsBase () const;
	virtual bool IsString () const;

	virtual DType* Clone() const;
	virtual DType* Deref() const;		// these return a new instance to the requested type...

	// for base types
	virtual DTypeEncoding Encoding() const;
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int ix, string& name, DType*& type, DLocationString& location, bool& subclass, bool& isVirtual) const;
	
	// for enumerations
	virtual bool GetNthEnumInfo (int ix, string& name, uint32& value) const;
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32 ix, int32& lower_bound, int32& upper_bound) const;
	virtual DType* GetElementType () const;

  private:

	void Init();

	class member_iterator : public iterator<input_iterator_tag, dwarf::entry>
	{
	  public:
		member_iterator ()
			: st (dwarf::entry()) {}
		member_iterator (const member_iterator& mi)
			: st (mi.st), si (mi.si)	{}
		member_iterator (const dwarf::entry& e, const dwarf::entry::sibling_iterator &i)
			: st (e), si (i) {}
		member_iterator& operator ++ ()
		{
			if (si != st.child_end())
			{
				do	++si;
				while (
					si != st.child_end() &&
					(*si).tag() != DW_TAG_member &&
					(*si).tag() != DW_TAG_inheritance);
			}
			return (*this);
		}
		member_iterator operator ++ (int)
		{
			member_iterator mi = *this;
			++(*this);
			return mi;
		}
		dwarf::entry operator * ()
		{
			return *si;
		}
		bool operator== (const member_iterator& i) const
		{
			return si == i.si;
		}
		bool operator!= (const member_iterator& i) const
		{
			return si != i.si;
		}
	  private:
	  	const dwarf::entry				&st;
		dwarf::entry::sibling_iterator	si;
	};
	
	member_iterator member_begin () const
	{
		member_iterator mi (fEntry, fEntry.child_begin ());

		if (mi != member_end())
		{
			uint32 tag = ((dwarf::entry)*mi).tag();
			if (tag != DW_TAG_member && tag != DW_TAG_inheritance)
				++mi;
		}

		return mi;
	}
	member_iterator member_end () const
		{ return member_iterator (fEntry, fEntry.child_end ()); }

	class enum_iterator : public iterator<input_iterator_tag, dwarf::entry>
	{
	  public:
		enum_iterator ()
			: st (NULL) {}
		enum_iterator (const enum_iterator& ei)
			: st (ei.st), si (ei.si)	{}
		enum_iterator (const dwarf::entry *e, const dwarf::entry::sibling_iterator &i)
			: st (e), si (i) {}
		enum_iterator& operator ++ ()
		{
			if (si != st->child_end())
			{
				do	++si;
				while (si != st->child_end() && (*si).tag() != DW_TAG_enumerator);
			}
			return (*this);
		}
		enum_iterator operator ++ (int)
		{
			enum_iterator ei = *this;
			++(*this);
			return ei;
		}
		dwarf::entry operator * ()
		{
			return *si;
		}
		bool operator== (const enum_iterator& i) const
		{
			return si == i.si;
		}
		bool operator!= (const enum_iterator& i) const
		{
			return si != i.si;
		}
	  private:
	  	const dwarf::entry				*st;
		dwarf::entry::sibling_iterator	si;
	};
	
	enum_iterator enum_begin () const
	{
		enum_iterator ei (&fEntry, fEntry.child_begin ());

		while (ei != enum_end() && ((dwarf::entry)*ei).tag() != DW_TAG_enumerator)
			++ei;

		return ei;
	}
	enum_iterator enum_end () const
		{ return enum_iterator (&fEntry, fEntry.child_end ()); }
	
	class subrange_iterator : public iterator<input_iterator_tag, dwarf::entry>
	{
	  public:
		subrange_iterator ()
			: st (dwarf::entry()) {}
		subrange_iterator (const subrange_iterator& ei)
			: st (ei.st), si (ei.si)	{}
		subrange_iterator (const dwarf::entry& e, const dwarf::entry::sibling_iterator &i)
			: st (e), si (i) {}
		subrange_iterator& operator ++ ()
		{
			if (si != st.child_end())
			{
				do	++si;
				while (si != st.child_end() && (*si).tag() != DW_TAG_subrange_type);
			}
			return (*this);
		}
		subrange_iterator operator ++ (int)
		{
			subrange_iterator ei = *this;
			++(*this);
			return ei;
		}
		dwarf::entry operator * ()
		{
			return *si;
		}
		bool operator== (const subrange_iterator& i) const
		{
			return si == i.si;
		}
		bool operator!= (const subrange_iterator& i) const
		{
			return si != i.si;
		}
	  private:
	  	const dwarf::entry				&st;
		dwarf::entry::sibling_iterator	si;
	};
	
	subrange_iterator subrange_begin () const
	{
		subrange_iterator ei (fEntry, fEntry.child_begin ());

		while (ei != subrange_end() && ((dwarf::entry)*ei).tag() != DW_TAG_subrange_type)
			++ei;

		return ei;
	}
	subrange_iterator subrange_end () const
		{ return subrange_iterator (fEntry, fEntry.child_end ()); }

	dwarf::entry fEntry;
	string fFullName;
	bool fString;
	DTypeEncoding fEncoding;
};

#endif
