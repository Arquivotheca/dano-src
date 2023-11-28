/*	$Id: DVariable.h,v 1.11 1999/05/03 13:09:59 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:57:53
*/

#ifndef DVARIABLE_H
#define DVARIABLE_H

#include "DType.h"
#include "DListBox.h"
#include "DLocation.h"

class DStackFrame;
class DNub;

enum EVarFormat {
	fmtDefault,
	fmtSigned,
	fmtUnsigned,
	fmtHex,
	fmtOctal,
	fmtChar,
	fmtString,
	fmtFloat,
	fmtEnum,
	fmtBool
};

class DVariable
{
  public:
  	DVariable () {}
  		// WARNING: DVariable will take over type and dispose it when it is disposed itself.
	DVariable (const string& name, DType *type, const DLocationString& location);
	virtual ~DVariable ();
	
	typedef std::vector<DVariable*>::const_iterator member_iterator;
	typedef std::vector<DVariable*>::const_reverse_iterator member_reverse_iterator;
	
	member_iterator member_begin() const					{ return fMembers.begin(); }
	member_iterator member_end() const					{ return fMembers.end(); }
	member_reverse_iterator member_rbegin() const		{ return fMembers.rbegin(); }
	member_reverse_iterator member_rend() const		{ return fMembers.rend(); }
	int member_count() const									{ return fMembers.size(); }

	DVariable* GetMemberByName(const char *name);

	// context independant member functions
	
	virtual void GetName (string& name) const;
	virtual void SetName (const string& name);

		// Get the value of a variable, pass in a buffer and the size of this buffer
		// size will be set to the actual size
	virtual void GetValue(const DStackFrame& frame, void *data, uint32& size) const;
	virtual void GetValue(const DStackFrame& frame, ptr_t& addr) const;	// to get the value of a pointer

	virtual void GetValueAsText(const DStackFrame& frame, string& value, EVarFormat format) const;
	virtual void GetValueAsText(DNub& nub, ptr_t addr, string& value, EVarFormat format) const;

	virtual void SetValue(DStackFrame& frame, const void *data, uint32 size) const;

	virtual void SetValueAsText(DStackFrame& frame, const string& value, EVarFormat format) const;
	virtual void SetValueAsText(DNub& nub, ptr_t base, const string& value, EVarFormat format) const;
	
	virtual DType* Type () const;
	virtual EVarFormat DefaultFormat() const;
	
	virtual ptr_t GetLocation (const DStackFrame& frame) const;
	virtual ptr_t GetLocation (DNub& nub, ptr_t base) const;

		// get the location state machine in DWARF-2 format
	virtual void GetLocationString (DLocationString& loc) const;
	
	virtual bool IsExpandable () const;
	virtual void Expand ();
//	virtual void Collapse ();
	
	bool operator== (const DVariable& var);

  protected:
  	
  	void SetValue(DStackFrame *frame, DNub *nub, ptr_t addr, const void *data, uint32 size) const;
  	void SetValueAsText(DStackFrame *frame, DNub *nub, ptr_t addr, const string& value, EVarFormat format) const;
  	
	void FormatBaseType(void *data, uint32 size, string& value, EVarFormat format) const;
	void FormatString(ptr_t addr, DNub& nub, string& value) const;

	string						fName;
	DLocationString			fLocation;
	DType					*fType;
	std::vector<DVariable*>	fMembers;
};

class DMemberVariable : public DVariable
{
  public:
	DMemberVariable (const DVariable& parent, const string& name, DType *type, const DLocationString& location)
		: DVariable (name, type, location), fParent (parent) {}

	virtual void GetValue (const DStackFrame& frame, void *value, uint32& size) const;
	virtual ptr_t GetLocation(const DStackFrame& frame) const;
	virtual ptr_t GetLocation(DNub& nub, ptr_t base) const;

	virtual void GetLocationString (DLocationString& loc) const;

  private:
	const DVariable&		fParent;
};

class DInheritedVariable : public DVariable
{
  public:
	DInheritedVariable (const DVariable& parent, const string& name, DType *type, const DLocationString& location)
		: DVariable (name, type, location), fParent (parent) {}

//	virtual void GetValue (const DStackFrame& frame, void *value, uint32& size) const;
	virtual ptr_t GetLocation (const DStackFrame& frame) const;
	virtual ptr_t GetLocation (DNub& nub, ptr_t base) const;

	virtual void GetLocationString (DLocationString& loc) const;

  private:
	const DVariable&		fParent;
};

class DPointerVariable : public DVariable
{
  public:
	DPointerVariable(DVariable& pointer);
	
	virtual void GetName (string& name) const;
	virtual ptr_t GetLocation (const DStackFrame& frame) const;
	virtual ptr_t GetLocation (DNub& nub, ptr_t base) const;
	
	virtual void GetLocationString (DLocationString& loc) const;

	virtual void GetValue(const DStackFrame& frame, void *data, uint32& size) const;

  private:
	DVariable& fPointer;
};

class DArrayVariable : public DVariable
{
  public:
	DArrayVariable (DVariable& parent, uint32 index, DType *type)
		: DVariable("", type, DLocationString())
		, fParent(parent)
		, fIndex(index) {}
	
	virtual void GetName (string& name) const;
	virtual void GetValue (const DStackFrame& frame, void *value, uint32& size) const;
	virtual ptr_t GetLocation (const DStackFrame& frame) const;
	virtual ptr_t GetLocation (DNub& nub, ptr_t base) const;

	virtual void GetLocationString (DLocationString& loc) const;

  private:
	DVariable& fParent;
	uint32 fIndex;
};

// A DConstVariable is used to store results in expression evaluation
// It is not an lvalue and therefore has no locationstring
class DConstVariable : public DVariable
{
  public:
	DConstVariable(const string& name, DType *type, const void *data);
	~DConstVariable();
	
	virtual void GetValue(const DStackFrame& frame, void *data, uint32& size) const;
	virtual ptr_t GetLocation(const DStackFrame& frame) const;
	virtual ptr_t GetLocation(DNub& nub, ptr_t base) const;
	virtual void GetLocationString (DLocationString& loc) const;

  private:
	char *fData;
};

inline void DVariable::GetValue(const DStackFrame& frame, ptr_t& addr) const
{
	uint32 size = sizeof(ptr_t);
	GetValue(frame, &addr, size);
} // DVariable::GetValue

#endif
