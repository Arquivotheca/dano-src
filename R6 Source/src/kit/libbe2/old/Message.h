/******************************************************************************
/
/	File:			Message.h
/
/	Description:	BMessage class creates objects that store data and that
/					can be processed in a message loop.  BMessage objects
/					are also used as data containers by the archiving and 
/					the scripting mechanisms.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _OLD_MESSAGE_H
#define _OLD_MESSAGE_H

#include <support2/SupportDefs.h>

#include <kernel/OS.h>
#include <render2/Rect.h>
#include <support2/Flattenable.h>

class BParcel;

namespace std {
	struct nothrow_t;
}

struct entry_ref;

namespace B {
namespace Support2 {
class BDirectMessageTarget;
class BValue;
}

namespace Old {

using namespace Support2;
using namespace Render2;

namespace BPrivate {
	struct message_target;
	struct message_write_context;
	struct header_args;
	class BMessageBody;
	class BSyncReplyTarget;
}

/*-------------------------------------------------------------*/
/* ------- Name lengths and Scripting specifiers ------------- */

#define B_FIELD_NAME_LENGTH			255
#define B_PROPERTY_NAME_LENGTH		255

// These are the types of flattened BMessage formats.
enum message_version {
	B_MESSAGE_VERSION_ANY		= 0,					// Don't care.
	B_MESSAGE_VERSION_1			= 1,					// R5 and before.
	B_MESSAGE_VERSION_2			= 2,					// Post-R5.
	B_MESSAGE_VERSION_CURRENT	= B_MESSAGE_VERSION_2
};

/*-------------------------------------------------------------*/
/* --------- BMessage class----------------------------------- */

class BMessage {

public:
		uint32		what;

					BMessage();
					BMessage(uint32 what);
					BMessage(const BMessage &a_message);
					BMessage(const BValue &a_value);
virtual				~BMessage();

		BMessage	&operator=(const BMessage &msg);

		BValue		AsValue() const;
		
/* Mass updates */
		status_t	Update(const BMessage& from, bool recursive = false);
		status_t	FillIn(const BMessage& from, bool recursive = false);
		
/* Statistics and misc info */
		status_t	GetInfo(type_code typeRequested, int32 which, const char **name,
							type_code *typeReturned, int32 *count = NULL) const;

		status_t	GetInfo(const char *name, type_code *type, int32 *c = 0) const;
		status_t	GetInfo(const char *name, type_code *type, bool *fixed_size) const;
		status_t	GetNextName(void **cookie, const char **outName,
								type_code *outType=NULL, int32 *outCount=NULL) const;

		int32		CountNames(type_code type) const;
		bool		IsEmpty() const;
		bool		IsSystem() const;
		bool		IsReply() const;
		void		PrintToStream() const;

		status_t	Rename(const char *old_entry, const char *new_entry);

/* Delivery info */

		int32		Target();

		bool		WasDelivered() const;
		bool		IsReplyRequested() const;
		bool		IsSourceWaiting() const;
		bool		IsSourceRemote() const;
		const BMessage	*Previous() const;
		bool		WasDropped() const;
		BPoint		DropPoint(BPoint *offset = NULL) const;

/* Time stamp */
		bool		HasWhen() const;
		bigtime_t	When() const;
		void		SetWhen(bigtime_t time);
		
/* Flattening data */
		ssize_t		FlattenedSize() const;
		status_t	Flatten(char *buffer, ssize_t size) const;
		status_t	Unflatten(const char *flat_buffer);

		ssize_t		FlattenedSize(message_version format) const;
		status_t	Flatten(message_version format, char *buffer, ssize_t size) const;

/* Flattening and unflattening with raw ports */
		status_t	WritePort(port_id port, int32 code,
							  uint32 flags=0, bigtime_t timeout=B_INFINITE_TIMEOUT) const;
		status_t	ReadPort(port_id port, ssize_t size=-1, int32* outCode=NULL,
							 uint32 flags=0, bigtime_t timeout=B_INFINITE_TIMEOUT);
		size_t		RawPortSize() const;
		const void*	RawPortData() const;
		
/* Adding data */
		status_t	AddRect(const char *name, BRect a_rect);
		status_t	AddPoint(const char *name, BPoint a_point);
		status_t	AddString(const char *name, const char *a_string);
		status_t	AddString(const char *name, const BString& a_string);
		status_t	AddInt8(const char *name, int8 val);
		status_t	AddInt16(const char *name, int16 val);
		status_t	AddInt32(const char *name, int32 val);
		status_t	AddInt64(const char *name, int64 val);
		status_t	AddBool(const char *name, bool a_boolean);
		status_t	AddFloat(const char *name, float a_float);
		status_t	AddDouble(const char *name, double a_double);
//		status_t	AddRGBColor(const char* name, rgb_color a_color,
//								type_code type = B_RGB_COLOR_TYPE);
		status_t	AddPointer(const char *name, const void *ptr);
		status_t	AddRef(const char *name, const entry_ref *ref);
		status_t	AddMessage(const char *name, const BMessage &msg);
		status_t	AddFlat(const char *name, const BFlattenable *obj, int32 count = 1);
		status_t	AddAtom(const char *name, const BAtom* atom);
		status_t	AddAtomRef(const char *name, const BAtom* atom);

		status_t	AddData(const char *name, type_code type, const void *data,
						ssize_t numBytes, bool is_fixed_size = true, int32 count = 1);


/* Removing data */
		status_t	RemoveData(const char *name, int32 index = 0);
		status_t	RemoveName(const char *name);
		status_t	MakeEmpty();

/* Finding data */
		status_t	FindRect(const char *name, BRect &rect) const;
		status_t	FindRect(const char *name, int32 index, BRect &rect) const;
		status_t	FindPoint(const char *name, BPoint &pt) const;
		status_t	FindPoint(const char *name, int32 index, BPoint &pt) const;
		status_t	FindString(const char *name, const char **str) const;
		status_t	FindString(const char *name, int32 index, const char **str) const;
		status_t	FindString(const char *name, BString &str) const;
		status_t	FindString(const char *name, int32 index, BString &str) const;
		status_t	FindInt8(const char *name, int8 *value) const;
		status_t	FindInt8(const char *name, int32 index, int8 *val) const;
		status_t	FindInt16(const char *name, int16 *value) const;
		status_t	FindInt16(const char *name, int32 index, int16 *val) const;
		status_t	FindInt32(const char *name, int32 *value) const;
		status_t	FindInt32(const char *name, int32 index, int32 *val) const;
		status_t	FindInt64(const char *name, int64 *value) const;
		status_t	FindInt64(const char *name, int32 index, int64 *val) const;
		status_t	FindBool(const char *name, bool *value) const;
		status_t	FindBool(const char *name, int32 index, bool *value) const;
		status_t	FindFloat(const char *name, float *f) const;
		status_t	FindFloat(const char *name, int32 index, float *f) const;
		status_t	FindDouble(const char *name, double *d) const;
		status_t	FindDouble(const char *name, int32 index, double *d) const;
//		status_t	FindRGBColor(const char *name, rgb_color *c,
//								 bool allow_int32_type = false) const;
//		status_t	FindRGBColor(const char *name, int32 index, rgb_color *c,
//								 bool allow_int32_type = false) const;
		status_t	FindAtom(const char *name, BAtom** atom) const;
		status_t	FindAtom(const char *name, int32 index, BAtom** atom) const;
		status_t	FindAtomRef(const char *name, BAtom** atom) const;
		status_t	FindAtomRef(const char *name, int32 index, BAtom** atom) const;
		status_t	FindPointer(const char *name, void **ptr) const;
		status_t	FindPointer(const char *name, int32 index,  void **ptr) const;
		status_t	FindRef(const char *name, entry_ref *ref) const;
		status_t	FindRef(const char *name, int32 index, entry_ref *ref) const;
		status_t	FindMessage(const char *name, BMessage &msg) const;
		status_t	FindMessage(const char *name, int32 index, BMessage &msg) const;
		status_t	FindFlat(const char *name, BFlattenable *obj) const;
		status_t	FindFlat(const char *name, int32 index, BFlattenable *obj) const;
		status_t	FindData(const char *name, type_code type,
							const void **data, ssize_t *numBytes) const;
		status_t	FindData(const char *name, type_code type, int32 index,
							const void **data, ssize_t *numBytes) const;

/* Replacing data */
		status_t	ReplaceRect(const char *name, BRect a_rect);
		status_t	ReplaceRect(const char *name, int32 index, BRect a_rect);
		status_t	ReplacePoint(const char *name, BPoint a_point);
		status_t	ReplacePoint(const char *name, int32 index, BPoint a_point);
		status_t	ReplaceString(const char *name, const char *string);
		status_t	ReplaceString(const char *name, int32 index, const char *string);
		status_t	ReplaceString(const char *name, const BString& string);
		status_t	ReplaceString(const char *name, int32 index, const BString& string);
		status_t	ReplaceInt8(const char *name, int8 val);
		status_t	ReplaceInt8(const char *name, int32 index, int8 val);
		status_t	ReplaceInt16(const char *name, int16 val);
		status_t	ReplaceInt16(const char *name, int32 index, int16 val);
		status_t	ReplaceInt32(const char *name, int32 val);
		status_t	ReplaceInt32(const char *name, int32 index, int32 val);
		status_t	ReplaceInt64(const char *name, int64 val);
		status_t	ReplaceInt64(const char *name, int32 index, int64 val);
		status_t	ReplaceBool(const char *name, bool a_bool);
		status_t	ReplaceBool(const char *name, int32 index, bool a_bool);
		status_t	ReplaceFloat(const char *name, float a_float);
		status_t	ReplaceFloat(const char *name, int32 index, float a_float);
		status_t	ReplaceDouble(const char *name, double a_double);
		status_t	ReplaceDouble(const char *name, int32 index, double a_double);
//		status_t	ReplaceRGBColor(const char *name, rgb_color a_color,
//									bool allow_int32_type = false);
//		status_t	ReplaceRGBColor(const char *name, int32 index, rgb_color a_color,
//									bool allow_int32_type = false);
		status_t	ReplaceAtom(const char *name, const BAtom* atom);
		status_t	ReplaceAtom(const char *name,int32 index, const BAtom* atom);
		status_t	ReplaceAtomRef(const char *name, const BAtom* atom);
		status_t	ReplaceAtomRef(const char *name,int32 index, const BAtom* atom);
		status_t	ReplacePointer(const char *name, const void *ptr);
		status_t	ReplacePointer(const char *name,int32 index,const void *ptr);
		status_t	ReplaceRef(	const char *name,const entry_ref *ref);
		status_t	ReplaceRef(	const char *name, int32 index, const entry_ref *ref);
		status_t	ReplaceMessage(const char *name, const BMessage &msg);
		status_t	ReplaceMessage(const char *name, int32 index, const BMessage &msg);
		status_t	ReplaceFlat(const char *name, const BFlattenable *obj);
		status_t	ReplaceFlat(const char *name, int32 index, const BFlattenable *obj);
		status_t	ReplaceData(const char *name, type_code type,
								const void *data, ssize_t data_size);
		status_t	ReplaceData(const char *name, type_code type, int32 index,
								const void *data, ssize_t data_size);

		void		*operator new(size_t size);
		void		*operator new(size_t size, const std::nothrow_t&);
		void		*operator new(size_t, void * arg);
		void		operator delete(void *ptr, size_t size);

/*----- Private, reserved, or obsolete ------------------------------*/
		bool		HasRect(const char *, int32 n = 0) const;
		bool		HasPoint(const char *, int32 n = 0) const;
		bool		HasString(const char *, int32 n = 0) const;
		bool		HasInt8(const char *, int32 n = 0) const;
		bool		HasInt16(const char *, int32 n = 0) const;
		bool		HasInt32(const char *, int32 n = 0) const;
		bool		HasInt64(const char *, int32 n = 0) const;
		bool		HasBool(const char *, int32 n = 0) const;
		bool		HasFloat(const char *, int32 n = 0) const;
		bool		HasDouble(const char *, int32 n = 0) const;
		bool		HasRGBColor(const char *name, int32 n = 0,
								bool allow_int32_type = false) const;
		bool		HasAtom(const char *, int32 n = 0) const;
		bool		HasAtomRef(const char *, int32 n = 0) const;
		bool		HasPointer(const char *, int32 n = 0) const;
		bool		HasMessenger(const char *, int32 n = 0) const;
		bool		HasRef(const char *, int32 n = 0) const;
		bool		HasMessage(const char *, int32 n = 0) const;
		bool		HasFlat(const char *, const BFlattenable *) const;
		bool		HasFlat(const char *,int32 ,const BFlattenable *) const;
		bool		HasData(const char *, type_code , int32 n = 0) const;
		BRect		FindRect(const char *, int32 n = 0) const;
		BPoint		FindPoint(const char *, int32 n = 0) const;
		const char	*FindString(const char *, int32 n = 0) const;
		int8		FindInt8(const char *, int32 n = 0) const;
		int16		FindInt16(const char *, int32 n = 0) const;
		int32		FindInt32(const char *, int32 n = 0) const;
		int64		FindInt64(const char *, int32 n = 0) const;
		bool		FindBool(const char *, int32 n = 0) const;
		float		FindFloat(const char *, int32 n = 0) const;
		double		FindDouble(const char *, int32 n = 0) const;

		status_t	AddAtom(const char *name, const atom_ptr<BAtom> &obj);
		status_t	AddAtomRef(const char *name, const atom_ptr<BAtom> &obj);
		status_t	AddAtomRef(const char *name, const atom_ref<BAtom> &obj);
		status_t	FindAtom(const char *name, atom_ptr<BAtom> &atom) const;
		status_t	FindAtom(const char *name, int32 index, atom_ptr<BAtom> &atom) const;
		status_t	FindAtomRef(const char *name, atom_ref<BAtom> &atom) const;
		status_t	FindAtomRef(const char *name, int32 index, atom_ref<BAtom> &atom) const;
		status_t	ReplaceAtom(const char *name, const atom_ptr<BAtom> &atom);
		status_t	ReplaceAtom(const char *name,int32 index, const atom_ptr<BAtom> &atom);
		status_t	ReplaceAtomRef(const char *name, const atom_ref<BAtom> &atom);
		status_t	ReplaceAtomRef(const char *name,int32 index, const atom_ref<BAtom> &atom);
		template <class TYPE> inline status_t	FindAtom(const char *name, atom_ptr<TYPE> &atom) const;
		template <class TYPE> inline status_t	FindAtom(const char *name, int32 index, atom_ptr<TYPE> &atom) const;
		template <class TYPE> inline status_t	FindAtomRef(const char *name, atom_ref<TYPE> &atom) const;
		template <class TYPE> inline status_t	FindAtomRef(const char *name, int32 index, atom_ref<TYPE> &atom) const;

private:

// optimization for gcc
#if __GNUC__
#define STANDARD_CALL __attribute__((stdcall))
#define ARITHMETIC_CALL __attribute__((stdcall,const))
#else
#define STANDARD_CALL
#define ARITHMETIC_CALL
#endif

friend class	BMessageList;
friend class	BMessenger;
friend class	BDispatcher;
friend class	::BParcel;

static			void		_msg_cache_cleanup_();
static			BMessage 	*_reconstruct_msg_(uint32,uint32);
friend inline	void		_set_message_target_(BMessage *, int32, bool);
friend inline	void		_set_message_reply_(BMessage *, BMessenger);
friend inline	int32		_get_message_target_(BMessage *);
friend inline	bool		_use_preferred_target_(BMessage *);
friend			ITextOutput::arg	operator<<(ITextOutput::arg io, const BMessage& message);

virtual	void		_ReservedMessage1();
virtual	void		_ReservedMessage2();
virtual	void		_ReservedMessage3();

		void		init_data() STANDARD_CALL;
		void		make_real_empty() STANDARD_CALL;
		
		BPrivate::BMessageBody* edit_body() STANDARD_CALL;
		BPrivate::BMessageBody* reset_body() STANDARD_CALL;
		
		void		get_args(BPrivate::header_args* into) const STANDARD_CALL;
		void		get_args(BPrivate::header_args* into, bigtime_t when) const STANDARD_CALL;
		void		set_args(const BPrivate::header_args* from) STANDARD_CALL;
		
		status_t	fast_add_data(const char *name, type_code type, const void *data,
									ssize_t numBytes, bool is_fixed_size=true) STANDARD_CALL;
		status_t	fast_find_data(const char *name, type_code type, int32 index,
									const void **data, ssize_t *numBytes) const STANDARD_CALL;
		status_t	fast_replace_data(const char *name, type_code type, int32 index,
										const void *data, ssize_t data_size) STANDARD_CALL;
		
		status_t	flatten_no_check(char *buffer, ssize_t size) const STANDARD_CALL;
		
		status_t	start_writing(BPrivate::message_write_context* context,
								  bool include_target=true, bool fixed_size=true) const STANDARD_CALL;
		void		finish_writing(BPrivate::message_write_context* context) const STANDARD_CALL;
		
static	ITextOutput::arg	print_message(ITextOutput::arg io, const BMessage & msg, int32 level);

		// avoid app_server depencies on BMessage's data fields.
		const BPrivate::message_target&	target_struct() const ARITHMETIC_CALL;
		BPrivate::message_target&		target_struct() ARITHMETIC_CALL;
		void							set_body(const BPrivate::BMessageBody* body) STANDARD_CALL;
		const BPrivate::BMessageBody*	body() const ARITHMETIC_CALL;
		void							set_read_only(bool state) STANDARD_CALL;
		bool							read_only() const ARITHMETIC_CALL;
		void							set_flatten_with_target(bool state) STANDARD_CALL;
		bool							flatten_with_target() const ARITHMETIC_CALL;
		
		enum				{ sNumReplyPorts = 3 };
static	BPrivate::BSyncReplyTarget*	sReplyPorts[sNumReplyPorts];
static	int32				sReplyPortInUse[sNumReplyPorts];
static	int32				sGetCachedReplyPort();

static	int					_init_message_();
static	int					_delete_message_();
static	BBlockCache*		sMsgCache;

#undef STANDARD_CALL
#undef ARITHMETIC_CALL

		BMessage			*fNext;
		BMessage			*fPrevious;
mutable	BMessage			*fOriginal;
		bigtime_t			fWhen;
		int32				fCurSpecifier;
		const BPrivate::BMessageBody	*fBody;
		uint32				fTarget[6];
		bool				fHasWhen;
		bool				fReadOnly;
		bool				fFlattenWithTarget;
		bool				_reservedBool;
};

template <class TYPE> inline status_t BMessage::FindAtom(const char *name, int32 index, atom_ptr<TYPE> &a) const
{
	BAtom* obj;
	status_t r = FindAtom(name,index,&obj);
	if (r == B_OK) {
		TYPE *t = dynamic_cast<TYPE*>(obj);
		if (t) a = t;
		else r = B_ERROR;
	};
	return r;
};

template <class TYPE> inline status_t BMessage::FindAtom(const char *name, atom_ptr<TYPE> &a) const
{
	return FindAtom(name,0,a);
};

template <class TYPE> inline status_t BMessage::FindAtomRef(const char *name, atom_ref<TYPE> &atom) const
{
	BAtom* obj;
	status_t r = FindAtomRef(name,index,&obj);
	if (r == B_OK) {
		TYPE *t = dynamic_cast<TYPE*>(obj);
		if (t) atom = t;
		else r = B_ERROR;
	};
	return r;
};

template <class TYPE> inline status_t BMessage::FindAtomRef(const char *name, int32 index, atom_ref<TYPE> &atom) const
{
	return FindAtomRef(name,0,atom);
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Old

#endif /* _OLD_MESSAGE_H */

