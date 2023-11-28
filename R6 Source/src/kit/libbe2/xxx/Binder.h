
#ifndef _SUPPORT2_BINDER_H
#define _SUPPORT2_BINDER_H

#include <OS.h>
#include <Atom.h>
#include <Dispatcher.h>
#include <Locker.h>
#include <SmartArray.h>
#include <String.h>

typedef struct binder_node_id binder_node_id;
typedef struct binder_cmd binder_cmd;

namespace BPrivate {
class uspace_io;
}

namespace B {
namespace Support2 {

class _link_data;
class BBinder;
class BBinderProxy;
class BBinderObserver;
class BBinderListener;
class BDispatcher;

typedef atom_ptr<class BBinder> binder;

struct put_status_t {
	status_t	error;
	bool		fallthrough		: 1;
	int32		reserved		: 31;

	put_status_t(status_t _error=0, bool _fallthrough=false) { error = _error; *(((int32*)&error)+1) = 0; fallthrough = _fallthrough; };
	inline operator status_t() const { return error; };
};

struct get_status_t {
	status_t	error;
	bool		resultCacheable	: 1;
	bool		ignoreArgs		: 1;
	int32		reserved		: 30;

	get_status_t(status_t _error=0, bool _cache=false) { error = _error; *(((int32*)&error)+1) = 0; resultCacheable = _cache; };
	inline operator status_t() const { return error; };
};

enum {
	B_SOMETHING_CHANGED		= 0x00000001,
	B_NAME_KNOWN			= 0x80000000,	// extraData = (const char *) property name

	B_NAMES_CHANGED			= 0x00000002 | B_SOMETHING_CHANGED,
	B_VALUES_CHANGED		= 0x00000004 | B_SOMETHING_CHANGED,

	B_PROPERTY_ADDED		= 0x00000010 | B_NAMES_CHANGED | B_VALUES_CHANGED | B_NAME_KNOWN,
	B_PROPERTY_REMOVED		= 0x00000020 | B_NAMES_CHANGED | B_VALUES_CHANGED | B_NAME_KNOWN,
	B_PROPERTY_CHANGED		= 0x00000040 | B_VALUES_CHANGED | B_NAME_KNOWN
};

#define BINDER_FUNCTION(funcName) 																	\
	property		funcName (const property_list &args);											\
	static property	funcName ## _binding (const property_list &args) { return funcName(args); }

#define BINDER_FUNCTOR(funcName) property(this,this,&(funcName ## _binding))

struct binder_typed_raw{
	size_t len;
	type_code type;
	char buffer[0];	
};

class BBinder : virtual public BAtom
{
	public:
				class 						property;
				class 						property_ref;
				class 						property_list;
				class 						iterator;
		typedef status_t 					(*observer_callback)(void *userData, uint32 observed, void *extraData);
		typedef int32						observer_token;

		static	property_list				empty_arg_list;

		static	property					Root();

											BBinder();
											BBinder(BDispatcher *dispatcher);
											BBinder(BBinder *copyFrom, bool deepCopy);
		virtual								~BBinder();

		virtual	void						Released();
		virtual	binder						Copy(bool deep=false);

				port_id						Port();
				int32						Token();
		static	atom_ref<BBinder>			LookupByToken(int32 token);

				BDispatcher *				Dispatcher();

		virtual	uint32						Flags() const;
				bool						IsValid() const { return Flags() & isValid; };
				bool						IsLocal() const { return Flags() & isLocal; };

				iterator					Properties();

				put_status_t				PutProperty(const char *name, const property &property);
				get_status_t				GetProperty(const char *name, property &property, const property_list &args = empty_arg_list);
				get_status_t				GetProperty(const char *name, property &property, const property *args, ...);
				property					Property(const char *path, property_list &args = empty_arg_list);
				property					Property(const char *path, const property *args, ...);

				status_t					StackOnto(binder object);
				status_t					Unstack();
				status_t					Topple();

				status_t					InheritFrom(binder object);
				status_t					RenounceAncestry();
				status_t					DisownChildren();

				status_t					Mount(const char *path);

				observer_token				AddObserverCallback(void *userData, observer_callback callbackFunc, uint32 observeMask, const char *name);
				status_t					RemoveObserverCallback(observer_token observer);

				void						AddObserver(const atom_ref<BBinderListener> &observer, uint32 observeMask, const char *name);
				status_t					RemoveObserver(const atom_ref<BBinderListener> &observer);
		enum {
			isValid = 0x00000001,
			isLocal = 0x00000001
		};

		class property_list : public SmartArray<property*>
		{
			public:

			property_list() {};
			property_list(int32 size) : SmartArray<property*>(size) {};
			inline property &	operator[](int i) const { return *((*((SmartArray<property*>*)this))[i]); };
		};
		
		class property
		{
			public:
		
											enum type {
												null = 0,
												string,
												number,
												object,
												remote_object,
												typed_raw
											};
		
											property();
											property(const property &);
											property(const property_ref &);

											property(const binder &value) { Init(value.ptr()); };
											property(class BBinder *value) { Init(value); };
											property(BString &value) { Init(value.String()); };
											property(const char *value) { Init(value); };
											property(double value) { Init(value); };
											property(int value) { Init((double)value); };
											property(int32 value) { Init((double)value); };
											property(int64 value) { Init((double)value); };
											property(type_code type, void* value, size_t len) { Init(type, value, len); };
//	for functor extension					property(class Binder *, void *, property (*)(const property_list &args));
											~property();
			
				type						Type() const;
				bool						IsUndefined() const;
				bool						IsNumber() const;
				bool						IsString() const;
				bool						IsObject() const;
				bool						IsTypedRaw() const;
				bool						IsRemoteObject() const;
		
				double						Number() const;
				BString						String() const;
				binder						Object() const;
				binder_typed_raw*			TypedRaw() const;
				
				void						Undefine();
		
				inline						operator BString() const { return String(); };
				inline						operator double() const { return Number(); };
				inline						operator binder() const { return Object(); };
				inline						operator binder_typed_raw *() const { return TypedRaw(); };
				
				bool						operator >=(const property &) const;
				bool						operator <=(const property &) const;
				bool						operator > (const property &) const;
				bool						operator < (const property &) const;
				bool						operator ==(const property &) const;
				bool						operator !=(const property &) const;
		
				const property &			operator =(const property &);
				property_ref				operator /(const char *);
				property_ref				operator [](const char *);
				BBinder * 					operator ->();
		
				int32						FlattenedSize() const;
				int32						FlattenTo(void *buffer) const;
				int32						UnflattenFrom(void *buffer);
		
				static property				undefined;
		
			private:
		
				friend BBinder;
				friend BBinderProxy;
				friend BDispatcher;
				friend BPrivate::uspace_io;
//				friend get_status_t BBinder::kGetProperty(uint32, const char *, BBinder::property &, const BBinder::property_list &);
//				friend put_status_t BBinder::kPutProperty(uint32, const char *, const BBinder::property &);
		
				void						Init(BBinder *value);
				void						Init(const char *value);
				void						Init(double value);
				void						Init(type_code type, void *value, size_t len);
				status_t					Remotize();
				status_t					InstantiateRemote();
		
				enum format {
					f_null = 0,
					f_string,
					f_number,
					f_object,
					f_descriptor,
					f_typed_raw
				};
				
				type m_type : 8;
				format m_format : 8;
				int32 m_reserved : 16;
				
				union {
					uint32 descriptor;
					char *string;
					double number;
					void *typed_raw;
					BBinder *object;
				} m_value;
		};

		class iterator {
			public:
											~iterator();
						BString 			Next();
				const	iterator &			operator =(const iterator &);
			private:
						friend				BBinder;
											iterator(BBinder *node);
						void *				cookie;
						binder				parent;
		};

		class property_ref {
		
			public:
				const property_ref &		operator =(const property &);

				put_status_t				Put(const property &prop) const;
				get_status_t				Get(property &returnVal) const;
				get_status_t				Get(property &returnVal, const property *, ...) const;
				get_status_t				Get(property &returnVal, const property_list &) const;

				property					operator ()() const;
				property					operator ()(const property *, ...) const;
				property					operator ()(const property_list &) const;

				double						Number() const { return (*this)().Number(); };
				BString						String() const { return (*this)().String(); };
				binder						Object() const { return (*this)().Object(); };
				binder_typed_raw*			TypedRaw() const { return (*this)().TypedRaw(); };
				
				inline						operator BString() const { return String(); };
				inline						operator double() const { return Number(); };
				inline						operator float() const { return (float)Number(); };
				inline						operator int() const { return (int)Number(); };
				inline						operator int32() const { return (int32)Number(); };
				inline						operator int64() const { return (int64)Number(); };
				inline						operator binder() const { return Object(); };
				inline						operator binder_typed_raw *() const { return TypedRaw(); };

				property_ref				operator /(const char *name) { return ((property)(*this)) / name; };
				property_ref				operator [](const char *name) { return ((property)(*this)) / name; };

				bool						operator >=(const property &p) const { return ((property)(*this)) >= p; };
				bool						operator <=(const property &p) const { return ((property)(*this)) <= p; };
				bool						operator > (const property &p) const { return ((property)(*this)) >  p; };
				bool						operator < (const property &p) const { return ((property)(*this)) <  p; };
				bool						operator ==(const property &p) const { return ((property)(*this)) == p; };
				bool						operator !=(const property &p) const { return ((property)(*this)) != p; };

											~property_ref();
		
			private:
											friend class property;
											property_ref(BBinder *, const char *);
		
				binder						m_base;
				char *						m_name;
		};

	protected:
	
		virtual	status_t					OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t					NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t					CloseProperties(void *cookie);

		virtual	put_status_t				WriteProperty(const char *name, const property &prop);
		virtual	get_status_t				ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

				status_t					NotifyListeners(uint32 event, const char *name);
		virtual	void						DoNotification(uint32 event, const char *name);

	private:

		friend								iterator;
		friend								property;
		friend								BDispatcher;
		friend								BBinderProxy;
		
				status_t					SetPrivateFlags(uint32, bool);
		virtual	status_t					StartHosting();
				void						Disconnect(void *);

				status_t					_link(uint32 linkFlags, binder node);
				status_t 					_unlink(uint32 linkFlags, binder node);

	public:
	
		virtual	status_t					WillRewritten(binder disgruntledAncestor, uint32 event, const char *name);

	private:

		virtual	void						BindMeUp2();
		virtual	void						BindMeUp3();
		virtual	void						BindMeUp4();
		virtual	void						BindMeUp5();
		virtual	void						BindMeUp6();
		virtual	void						BindMeUp7();
		virtual	void						BindMeUp8();

		static	void						ProcessMessage(void *buf, size_t buf_size, BLocker *buffer_lock = NULL);

		static	int32						g_defaultDispatcherCreated;
		static	dispatcher					g_defaultDispatcher;

				void						Init(BDispatcher *dispatcher);

		static	BBinder *					rootNode();
		static	status_t					handle_reflect_punt(struct binder_cmd *cmd);
		static	int							do_binder_command(struct binder_cmd *cmd, property *prop=NULL, const property_list *args=NULL);
		static	status_t					open_node(struct binder_node_id node_id, uint32 *node_handle, bool startHosting=false);
		static	get_status_t 				kGetProperty(uint32, const char *, BBinder::property &, const BBinder::property_list &);
		static	put_status_t				kPutProperty(uint32, const char *, const BBinder::property &);
		static	int 						kSeverLinks(int32 desc, uint32 linkFlags);

				atom_ptr<BDispatcher>		m_dispatcher;
				int32						m_token;
				uint32						m_hostingNodeHandle;
				BLocker						m_lock;
				_link_data *				m_linkData;
};

#define PROPERTY_NAME_LEN 64

enum {
	permsRead		= 0x0001,
	permsWrite		= 0x0002,
	permsCreate		= 0x0004,
	permsDelete		= 0x0008,
	permsMount 		= 0x0010,
	permsInherit	= 0x8000
};

class BBinderListener : virtual public BAtom
{
	public:
										BBinderListener();
		virtual							~BBinderListener();

				status_t				StartListening(binder node, uint32 eventMask=0xFFFFFFFF, const char *propertyName=NULL);
				status_t				StopListening(binder node);

		virtual	status_t				Overheard(binder node, uint32 observed, BString propertyName);
};

// The old-style observer
class BBinderObserver
{
	public:
										BBinderObserver(const binder &node, uint32 eventMask=0xFFFFFFFF, const char *name=NULL);
		virtual							~BBinderObserver();

		const	binder &				Object();
		virtual	status_t				ObservedChange(uint32 observed, const char *name);

	protected:
	
		static	status_t 				Callback(BBinderObserver *, uint32, void *);
	
		binder							m_object;
		BBinder::observer_token			m_token;
};

class BBinderContainer : public BBinder
{
	public:
										BBinderContainer();
										BBinderContainer(BBinderContainer *copyFrom, bool deepCopy);
		virtual							~BBinderContainer();

		virtual	uint32					Flags() const;

		virtual	status_t				OpenProperties(void **cookie, void *copyFrom);
		virtual	status_t				NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t				RewindProperties(void *cookie);
		virtual	status_t				CloseProperties(void *cookie);

		virtual	put_status_t			WriteProperty(const char *name, const property &prop);
		virtual	get_status_t			ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	status_t				WillRewritten(binder disgruntledAncestor, uint32 event, const char *name);

				status_t				AddProperty(const char *name, const property &prop, uint16 perms=permsInherit);
				status_t				RemoveProperty(const char *name);
				bool					HasProperty(const char *name);

				uint16					Permissions();
				status_t				SetPermissions(uint16 perms);

				bool					Ordered();
				status_t				SetOrdered(bool isOrdered);

		virtual	binder					Copy(bool deep=false);

	protected:
	
				bool					FindName(const char *name, int32 &index);

		struct property_record {
			char				name[PROPERTY_NAME_LEN];
			uint16				perms;
			property			value;
		};

		uint32							m_flags;
		SmartArray<property_record>		m_propertyList;
		BLocker							m_listLock;
		uint16							m_perms;
};

} } // namespace B::Support2

using namespace B::Support2;

#endif	/* _SUPPORT2_BINDER_H */
