#ifndef __MIME_EXTRA_ATTRIBUTES__
#define __MIME_EXTRA_ATTRIBUTES__

#ifndef _BE_H
#include <AppDefs.h>
#include <Window.h>
#include <ListItem.h>
#include <ListView.h>
#endif

extern const char *kTypeMessage;
extern const char *kAttributeName;

extern const char *kPrivateAttributeName; 	// "attr:name";
extern const char *kPublicAttributeName; 	// "attr:public_name";
extern const char *kAttributeType; 			// "attr:type";
extern const char *kAttributeEditable; 		// "attr:editable";
extern const char *kAttributeExtra; 		// "attr:extra";
extern const char *kAttributeEmail; 		// "extra:email";

class BMenuItem;
class BMenu;

class AttributeType {
	// maps human readable names to types and vice versa
	// builds a menu of types
public:
	AttributeType(int32 type, const char *name)
		:	type(type),
			typeName(name)
		{}	

	static const char *TypeName(int32 type);
	static int32 Type(const char *typeName);
	static void BuildTypeMenu(BMenu *);
private:
	BMenuItem *InstantiateMenuItem() const;

	int32 type;
	const char *typeName;

	static const AttributeType attributeTypes[];
};

class MIMEExtraAttribute {
public:
	MIMEExtraAttribute(int32 type, const char *name, const char *internalName,
		bool editable, bool viewable, bool extra, int32 width, int32 alignment);
	MIMEExtraAttribute(const BMessage *);
	MIMEExtraAttribute(const BMessage *, int32);
	virtual ~MIMEExtraAttribute();

	void Embed(BMessage *) const;
	
	bool IsValid() const;
	
	const char *Name() const;
	void SetName(const char *);
	
	int32 Type() const;
	void SetType(int32);
	const char *TypeName() const;

	const char *InternalName() const;
	void SetInternalName(const char *);
	
	bool Editable() const;
	void SetEditable(bool);
	
	bool Viewable() const;
	void SetViewable(bool);
	
	int32 TrackerViewWidth() const;
	void SetTrackerViewWidth(int32);

	int32 TrackerViewAlignment() const;
	void SetTrackerViewAlignment(int32);
	
	bool Extra() const;
	void SetExtra(bool);
private:
	void InitCommon(int32 type, const char *name, const char *internalName,
		bool editable, bool viewable, bool extra, int32 width, 
		int32 alignment);
		// constructors share code in this routine
		
	BMessage *fullAttribute;
};

class MIMEPanel;
class BMenuField;
class BTextControl;
class BButton;

class AttributeList : public BListView {
public:
	AttributeList(BRect, float divider = 0, uint32 resizeFlags = B_FOLLOW_ALL);
	virtual ~AttributeList();

	void SetTargetType(const BMimeType *);
		// called to setup AttributeList with data from the 
		// new mime type that was just selected

	void AddAttribute(const MIMEExtraAttribute *);
		// add attribute to list and mime data; does not check for uniqueness
	void RemoveSelectedAttribute();
		// remove selected atribute from list and mime data
	void ChangeCurrentAttribute(const MIMEExtraAttribute *);
		// remove selected atribute from list and mime data

	// Drawing stuff
	float Divider() const;
	void SetDivider(float);
	virtual	void Draw(BRect);

private:

	typedef BListView inherited;

	bool ApplyChange(BMimeType *);
		// sets the mime data file extra attributes to the current list
		// contents

	void ApplyCommon();

	float divider;
};

class AttributeListItem : public BStringItem {
public:
	AttributeListItem(const MIMEExtraAttribute *);
	virtual ~AttributeListItem();
	
	virtual	void DrawItem(BView *, BRect, bool complete = false);
	virtual void Update(BView *, const BFont *);

	const MIMEExtraAttribute *Attribute() const
		{ return attribute; }

	const char *InternalName() const;
	
	// no setter here handle changes by deleting and creating a new
	// AttributeListItem to work like BStringItem
private:
	typedef BStringItem inherited;
	const MIMEExtraAttribute *attribute;
	float whishIDidntHaveToDoThisBaselineOffset;
};

#endif
