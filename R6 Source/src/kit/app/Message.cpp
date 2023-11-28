//*****************************************************************************
//
//	File:		Message.cpp
//
//	
//	Written by:	Your Mom
//
//	Copyright 1994-97, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#include "MessageBody.h"

#include <Application.h>
#include <Atom.h>
#include <Autolock.h>
#include <BlockCache.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <Entry.h>
#include <Locker.h>
#include <Message.h>
#include <OS.h>
#include <Path.h> 
#include <StreamIO.h>
#include <String.h>
#include <StringIO.h>

#include <CallStack.h>
#include <message_util.h>
#include <token.h>

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <new>

static inline bool cmp_types(type_code t1, type_code t2)
{
	return (t1==t2) || (t1==B_ANY_TYPE) || (t2==B_ANY_TYPE);
}

#define TIMING 0

#if TIMING
#include <StopWatch.h>

// this is implemented in libroot.so
extern "C" {
extern int64	atomic_add64(vint64 *value, int64 addvalue);
}

static int32 gHasDebugLevel = 0;
static int32 gDebugLevel = 0;
static int32 MessageDebugLevel() {
	if ((gHasDebugLevel&2) != 0) return gDebugLevel;
	if (atomic_or(&gHasDebugLevel, 1) == 0) {
		const char* env = getenv("MESSAGE_DEBUG");
		if (env) {
			gDebugLevel = atoi(env);
			if (gDebugLevel < 1)
				gDebugLevel = 1;
		}
		atomic_or(&gHasDebugLevel, 2);
	}
	while ((gHasDebugLevel&2) == 0) sleep(2000);
	return gDebugLevel;
}

namespace BPrivate {
	struct timing_stats {
		int32 create_count;
		int32 delete_count;
		int32 empty_count;
		int32 copy_count;
		int32 flatten_count;
		int32 unflatten_count;
		int32 flatten_stream_count;
		int32 unflatten_stream_count;
		int32 add_count;
		int32 find_count;
		int32 replace_count;
		
		int64 empty_size;
		int64 copy_size;
		int64 flatten_size;
		int64 unflatten_size;
		int64 flatten_stream_size;
		int64 unflatten_stream_size;
		
		bigtime_t empty_time;
		bigtime_t copy_time;
		bigtime_t flatten_time;
		bigtime_t unflatten_time;
		bigtime_t flatten_stream_time;
		bigtime_t unflatten_stream_time;
		bigtime_t add_time;
		bigtime_t find_time;
		bigtime_t replace_time;
	};

	class TimeStat
	{
	public:
		TimeStat(int32 count, bigtime_t time)
			: fCount(count), fTime(time)
		{
		}
		~TimeStat()
		{
		}
	
		int32 Count() const			{ return fCount; }
		bigtime_t Time() const		{ return fTime; }
		
	private:
		int32 fCount;
		bigtime_t fTime;
	};
	
	static inline BDataIO& operator<<(BDataIO& io, const TimeStat& stat)
	{
		io << stat.Count();
		if (stat.Count() > 0) {
			io << " (" << ( ((double)stat.Time())/stat.Count() ) << " us/op)";
		} 
		return io;
	}
	
	class SizeStat
	{
	public:
		SizeStat(int32 count, int64 size)
			: fCount(count), fSize(size)
		{
		}
		~SizeStat()
		{
		}
	
		int32 Count() const			{ return fCount; }
		int64 Size() const			{ return fSize; }
		
	private:
		int32 fCount;
		int64 fSize;
	};
	
	static inline BDataIO& operator<<(BDataIO& io, const SizeStat& stat)
	{
		if (stat.Count() > 0) {
			io << ( ((double)stat.Size())/stat.Count() ) << " avg entries";
		} 
		return io;
	}
	
	class BMessageTiming
	{
	public:
		BMessageTiming()
		{
			memset(&fStats, 0, sizeof(fStats));
			fEmptyEmpty = false;
		}
		~BMessageTiming()
		{
			PrintStats(fStats);
		}
		
		void NoteCreate(BMessage* msg) {
			atomic_add(&fStats.create_count, 1);
			if (MessageDebugLevel() > 0) {
				BAutolock _l(fAccess);
				const int32 i = fMessages.IndexOf(msg);
				if (i >= 0) {
					BCallStack* stack = (BCallStack*)fStacks.ItemAt(i);
					BStringIO str;
					str << "Message " << msg << " already exists";
					if (stack) {
						str << endl << "Location of original creation:" << endl;
						stack->LongPrint(str);
					}
					debugger(str.String());
					return;
				}
				fMessages.AddItem(msg);
				BCallStack* stack = new BCallStack;
				stack->Update(0);
				fStacks.AddItem(stack);
			}
		}
		void NoteDelete(BMessage* msg) {
			atomic_add(&fStats.delete_count, 1);
			if (MessageDebugLevel() > 0) {
				BAutolock _l(fAccess);
				const int32 i = fMessages.IndexOf(msg);
				if (i < 0) {
					debugger("Message doesn't exist");
					return;
				}
				fMessages.RemoveItem(i);
				BCallStack* stack = (BCallStack*)fStacks.ItemAt(i);
				fStacks.RemoveItem(i);
				delete stack;
			}
		}
		
		void NoteEmpty(BMessage*, const BStopWatch& watch) {
			if (!fEmptyEmpty) {
				atomic_add(&fStats.empty_count, 1);
				atomic_add64(&fStats.empty_time, watch.ElapsedTime());
			} else {
				fEmptyEmpty = false;
			}
		}
		
		void NoteEmpty(BMessage*, int32 size) {
			if (size > 0) {
				atomic_add64(&fStats.empty_size, size);
				fEmptyEmpty = false;
			} else {
				fEmptyEmpty = true;
			}
		}
		
		void NoteCopy(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.copy_count, 1);
			atomic_add64(&fStats.copy_time, watch.ElapsedTime());
		}
		
		void NoteCopy(BMessage*, int32 size) {
			atomic_add64(&fStats.copy_size, size);
		}
		
		void NoteFlatten(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.flatten_count, 1);
			atomic_add64(&fStats.flatten_time, watch.ElapsedTime());
		}
		
		void NoteFlatten(BMessage*, int32 size) {
			atomic_add64(&fStats.flatten_size, size);
		}
		
		void NoteUnflatten(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.unflatten_count, 1);
			atomic_add64(&fStats.unflatten_time, watch.ElapsedTime());
		}
		
		void NoteUnflatten(BMessage*, int32 size) {
			atomic_add64(&fStats.unflatten_size, size);
		}
		
		void NoteFlattenStream(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.flatten_stream_count, 1);
			atomic_add64(&fStats.flatten_stream_time, watch.ElapsedTime());
		}
		
		void NoteFlattenStream(BMessage*, int32 size) {
			atomic_add64(&fStats.flatten_stream_size, size);
		}
		
		void NoteUnflattenStream(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.unflatten_stream_count, 1);
			atomic_add64(&fStats.unflatten_stream_time, watch.ElapsedTime());
		}
		
		void NoteUnflattenStream(BMessage*, int32 size) {
			atomic_add64(&fStats.unflatten_stream_size, size);
		}
		
		void NoteAdd(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.add_count, 1);
			atomic_add64(&fStats.add_time, watch.ElapsedTime());
		}
		
		void NoteFind(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.find_count, 1);
			atomic_add64(&fStats.find_time, watch.ElapsedTime());
		}
		
		void NoteReplace(BMessage*, const BStopWatch& watch) {
			atomic_add(&fStats.replace_count, 1);
			atomic_add64(&fStats.replace_time, watch.ElapsedTime());
		}
		
		void PrintStats(const timing_stats& stats) const
		{
			if (MessageDebugLevel() <= 0)
				return;
				
			if (stats.create_count || stats.delete_count) {
				BErr << "BMessage Statistics:" << endl;
				BErr << "Instances Created   : " << stats.create_count
					 << " / Deleted: " << stats.delete_count << endl;
				BErr << "Empty operations    : "
					 << TimeStat(stats.empty_count, stats.empty_time) << " / "
					 << SizeStat(stats.empty_count, stats.empty_size) << endl;
				BErr << "Copy operations     : "
					 << TimeStat(stats.copy_count, stats.copy_time) << " / "
					 << SizeStat(stats.copy_count, stats.copy_size) << endl;
				BErr << "Flatten operations  : "
					 << TimeStat(stats.flatten_count, stats.flatten_time) << " / "
					 << SizeStat(stats.flatten_count, stats.flatten_size) << endl;
				BErr << "Flatten streams     : "
					 << TimeStat(stats.flatten_stream_count, stats.flatten_stream_time) << " / "
					 << SizeStat(stats.flatten_stream_count, stats.flatten_stream_size) << endl;
				BErr << "Unflatten operations: "
					 << TimeStat(stats.unflatten_count, stats.unflatten_time) << " / "
					 << SizeStat(stats.unflatten_count, stats.unflatten_size) << endl;
				BErr << "Unflatten streams   : "
					 << TimeStat(stats.unflatten_stream_count, stats.unflatten_stream_time) << " / "
					 << SizeStat(stats.unflatten_stream_count, stats.unflatten_stream_size) << endl;
				BErr << "Add operations      : "
					 << TimeStat(stats.add_count, stats.add_time) << endl;
				BErr << "Find operations     : "
					 << TimeStat(stats.find_count, stats.find_time) << endl;
				BErr << "Replace operations  : "
					 << TimeStat(stats.replace_count, stats.replace_time) << endl;
			}
			if (fMessages.CountItems() > 0) {
				BErr << "Leaked " << fMessages.CountItems() << " messages:" << endl;
				for (int32 i=0; i<fMessages.CountItems(); i++) {
					BMessage* msg = (BMessage*)fMessages.ItemAt(i);
					BCallStack* stack = (BCallStack*)fStacks.ItemAt(i);
					BErr << "  Message " << msg << ":" << endl;
					if (stack) stack->LongPrint(BErr, NULL, "    ");
				}
			}
		}
	
	private:
		timing_stats fStats;
		BLocker fAccess;
		BList fMessages;
		BList fStacks;
		bool fEmptyEmpty;
	};
	
	static BMessageTiming Timing;
}

using namespace BPrivate;

#define NOTE_COUNT(msg, func) Timing.Note##func(msg)
#define NOTE_SIZE(msg, func, size) Timing.Note##func(msg, size)

#define START_TIMING() BStopWatch __stopwatch__("BMessage Stopwatch", true)
#define NOTE_TIMING(msg, func) Timing.Note##func(msg, __stopwatch__)
#define NOTE_TIMING_SIZE(msg, func, size) { Timing.Note##func(msg, __stopwatch__); Timing.Note##func(msg, size); }

#else

#define NOTE_COUNT(msg, func)
#define NOTE_SIZE(msg, func, size)

#define START_TIMING()
#define NOTE_TIMING(msg, func)
#define NOTE_TIMING_SIZE(msg, func, size)

#endif

/*---------------------------------------------------------------*/

namespace BPrivate {

class BSyncReplyTarget : public BDirectMessageTarget
{
public:
	BSyncReplyTarget(const char* port_name)
		: fCount(0),
		  fPort(create_port(1, port_name)),
		  fToken(gDefaultTokens->NewToken(REPLY_TOKEN_TYPE, (void*)fPort)),
		  fReplyMessage(NULL),
		  fReplyReceived(false)
	{
		AcquireTarget();
		// for now, disable direct message deposit.
		//gDefaultTokens->SetTokenTarget(fToken, this);
#if SUPPORTS_STREAM_IO
//		the line below is called in response to one of the init_before
//		functions. At that point it is not guaranteed that objects
//		with global scope are already constructed
//		BErr is a global object that definitely is not yet constructed
//		when using gcc3
//		BErr << "Creating BSyncReplyTarget " << this << endl;

		DEBUG_ONLY(fprintf(stderr,"Creating BSyncReplyTarget %p\n",this);)  
#endif
	}
	
	inline void Shutdown()
	{
		BAutolock _l(fAccess);
		fReplyMessage = NULL;
		fPort = B_BAD_PORT_ID;
	}
	
	inline void Disconnect()
	{
		if (gDefaultTokens && fToken != NO_TOKEN)
			gDefaultTokens->RemoveToken(fToken);
		Shutdown();
		ReleaseTarget();
	}
	
	inline port_id Port() const
	{
		return fPort;
	}
	
	inline int32 Token() const
	{
		return fToken;
	}
	
	inline void SetReplyMessage(BMessage* msg)
	{
		BAutolock _l(fAccess);
		fReplyMessage = msg;
		fReplyReceived = false;
	}
	
	inline bool ReplyReceived() const
	{
		return fReplyReceived;
	}
	
	virtual bool AcquireTarget()
	{
		atomic_add(&fCount, 1);
		return true;
	}
	
	virtual void ReleaseTarget()
	{
		if (atomic_add(&fCount, -1) == 1) {
			delete this;
		}
	}
	
	virtual status_t EnqueueMessage(BMessage* msg)
	{
		BAutolock _l(fAccess);
		if (fReplyMessage) {
			// Copy directly into reply message, and poke reply port
			// to wake up receiver.
			*fReplyMessage = *msg;
			fReplyReceived = true;
			write_port(fPort,0,NULL,0);
			return B_OK;
		}
		
//+		BErr << "Dropping message: " << *msg << endl;
		delete msg;
		return B_ERROR;
	}

protected:
	virtual ~BSyncReplyTarget()
	{
//+		BErr << "Deleting BSyncReplyTarget " << this << endl;
		Shutdown();
	}

private:
	int32 fCount;
	BLocker fAccess;
	port_id fPort;
	int32 fToken;
	BMessage* fReplyMessage;
	bool fReplyReceived;
};

}	// namespace BPrivate

using namespace BPrivate;

/* ----------------------------------------------------------------- */

static const char* IndentForLevel(int32 level)
{
	static const char space[] =
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int32 off = sizeof(space) - level - 1;
	if( off < 0 ) off = 0;
	return &space[off];
}

static inline int isident(int c)
{
	return isalnum(c) || c == '_';
}

static inline bool isasciitype(char c)
{
	if( c >= ' ' && c < 127 && c != '\'' && c != '\\' ) return true;
	return false;
}

static inline char makehexdigit(uint32 val)
{
	return "0123456789ABCDEF"[val&0xF];
}

static void appendhexnum(uint32 val, BString* out)
{
	const int32 base = out->Length();
	char* str = out->LockBuffer(base + 9);
	str += base;
	for( int32 i=7; i>=0; i-- ) {
		str[i] = makehexdigit( val );
		val >>= 4;
	}
	str[8] = 0;
	out->UnlockBuffer(base + 9);
}

static const char* UIntToHex(uint64 val, BString* out)
{
	char buf[16];
	sprintf(buf, "%p", (void*)val);
	*out = buf;
	return out->String();
}

static const char* TypeToString(type_code type, BString* out,
								bool fullContext = true,
								bool strict = false)
{
	const char c1 = (char)((type>>24)&0xFF);
	const char c2 = (char)((type>>16)&0xFF);
	const char c3 = (char)((type>>8)&0xFF);
	const char c4 = (char)(type&0xFF);
	bool valid;
	if( !strict ) {
		valid = isasciitype(c1) && isasciitype(c2) && isasciitype(c3) && isasciitype(c4);
	} else {
		valid = isident(c1) && isident(c2) && isident(c3) && isident(c4);
	}
	if( valid && (!fullContext || c1 != '0' || c2 != 'x') ) {
		char* s = out->LockBuffer(fullContext ? 7 : 5);
		int32 i = 0;
		if( fullContext ) s[i++] = '\'';
		s[i++] = c1;
		s[i++] = c2;
		s[i++] = c3;
		s[i++] = c4;
		if( fullContext ) s[i++] = '\'';
		out->UnlockBuffer(fullContext ? 6 : 4);
		return out->String();
	}
	
	if( fullContext ) *out = "0x";
	else *out = "";
	appendhexnum(type, out);
	
	return out->String();
}

BDataIO& BMessage::print_message(BDataIO& io, const BMessage& msg, int32 level)
{
#if SUPPORTS_STREAM_IO
	BString buf;
	
	io << "BMessage(" << TypeToString(msg.what, &buf) << ") {" << endl;
	
	level++;
	const char* prefix = IndentForLevel(level);
	
	message_target&	target = *(message_target*)(msg.fTarget);
	if (target.target != NO_TOKEN) {
		void* data = NULL;
		status_t r = B_ERROR;
		if (gDefaultTokens->CheckToken(target.target)) {
			r = gDefaultTokens->GetToken(target.target, NO_TOKEN_TYPE, &data);
		}
		if (target.flags&MTF_PREFERRED_TARGET)
			io << prefix << "// Target is preferred:";
		else
			io << prefix << "// Target is direct:";
		io << (void*)target.target;
		if (r >= B_OK) io << " (object " << data << ")";
		io << endl;
	} else if (target.flags&MTF_PREFERRED_TARGET) {
		io << prefix << "// Target is preferred handler" << endl;
	}
	if (target.flags&MTF_DELIVERED) {
		BMessenger reply(msg.ReturnAddress());
		io << prefix << "// Return to " << reply << endl;
	}
	if (target.flags&MTF_REPLY_SENT) {
		io << prefix << "// Reply has been sent" << endl;
	} else if (target.flags&MTF_SOURCE_IS_WAITING) {
		io << prefix << "// Sender is waiting for reply" << endl;
	} else if (target.flags&MTF_REPLY_REQUIRED) {
		io << prefix << "// Sender not waiting but requires a reply" << endl;
	}
	if (target.flags&MTF_IS_REPLY) {
		io << prefix << "// This is a reply to another message" << endl;
	}
	if (msg.fBody->HasSpecifiers()) {
		io << prefix << "// Current specifier is " << msg.fCurSpecifier << endl;
	}
	if (msg.fHasWhen) {
		io << prefix << "// Timestamp (when) at " << msg.fWhen << "us" << endl;
	}
	
	void *cookie = NULL;
	const char* name;
	type_code type;
	long count;
	while (msg.GetNextName(&cookie, &name, &type, &count) == B_OK) {
		for( int32 j=0; j<count; j++ ) {
			io << prefix << name;
			if( count > 1 ) io << "[" << j << "]";
			io << " = ";
			
			bool handled = false;
			if (type == B_MESSENGER_TYPE) {
				BMessenger obj;
				status_t err = msg.FindMessenger(name, j, &obj);
				if (err == B_OK) {
					io << obj;
					handled = true;
				}
			}
			
			const void* data = 0;
			ssize_t size = 0;
			if (!handled) {
				status_t err = msg.FindData(name, type, j, &data, &size);
				if (err != B_OK) {
					io << strerror(err) << endl;
					continue;
				}
			} else {
				type = 0;
			}
			
			if (data) {
				switch( type ) {
				case B_CHAR_TYPE: {
					if (size == sizeof(char)) {
						char c[2];
						c[0] = *(char*)data;
						c[1] = 0;
						io << "char(" << (int32)c[0]
						   << " or " << UIntToHex((uint8)c[0], &buf);
						if (isprint(c[0])) io << " or '" << c << "'";
						io << ")";
						handled = true;
					}
				} break;
				case B_INT8_TYPE: {
					if (size == sizeof(int8)) {
						int8 c[2];
						c[0] = *(int8*)data;
						c[1] = 0;
						io << "int8(" << (int32)c[0]
						   << " or " << UIntToHex((uint8)c[0], &buf);
						if (isprint(c[0])) io << " or '" << (char*)&c[0] << "'";
						io << ")";
						handled = true;
					}
				} break;
				case B_INT16_TYPE: {
					if (size == sizeof(int16)) {
						io << "int16(" << *(int16*)data
						   << " or " << UIntToHex(*(uint16*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_INT32_TYPE: {
					if (size == sizeof(int32)) {
						io << "int32(" << *(int32*)data
						   << " or " << UIntToHex(*(uint32*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_INT64_TYPE: {
					if (size == sizeof(int64)) {
						io << "int64(" << *(int64*)data
						   << " or " << UIntToHex(*(uint64*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_UINT8_TYPE: {
					if (size == sizeof(uint8)) {
						io << "uint8(" << (uint32)*(uint8*)data
						   << " or " << UIntToHex(*(uint8*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_UINT16_TYPE: {
					if (size == sizeof(uint16)) {
						io << "uint16(" << (uint32)*(uint16*)data
						   << " or " << UIntToHex(*(uint16*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_UINT32_TYPE: {
					if (size == sizeof(uint32)) {
						io << "uint32(" << *(uint32*)data
						   << " or " << UIntToHex(*(uint32*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_UINT64_TYPE: {
					if (size == sizeof(uint64)) {
						io << "uint64(" << *(uint64*)data
						   << " or " << UIntToHex(*(uint64*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_FLOAT_TYPE: {
					if (size == sizeof(float)) {
						io << "float(" << *(float*)data << ")";
						handled = true;
					}
				} break;
				case B_DOUBLE_TYPE: {
					if (size == sizeof(double)) {
						io << "double(" << *(double*)data << ")";
						handled = true;
					}
				} break;
				case B_BOOL_TYPE: {
					if (size == sizeof(bool)) {
						io << "bool(" << ((*(bool*)data) ? "true" : "false") << ")";
						handled = true;
					}
				} break;
				case B_OFF_T_TYPE: {
					if (size == sizeof(off_t)) {
						io << "off_t(" << *(off_t*)data
						   << " or " << UIntToHex(*(off_t*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_SIZE_T_TYPE: {
					if (size == sizeof(size_t)) {
						io << "size_t(" << *(size_t*)data
						   << " or " << UIntToHex(*(size_t*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_SSIZE_T_TYPE: {
					if (size == sizeof(ssize_t)) {
						io << "ssize_t(" << *(ssize_t*)data
						   << " or " << UIntToHex(*(size_t*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_POINTER_TYPE: {
					if (size == sizeof(void*)) {
						io << "pointer(" << UIntToHex(*(size_t*)data, &buf) << ")";
						handled = true;
					}
				} break;
				case B_MESSAGE_TYPE: {
					BMessage getit;
					status_t err = getit.Unflatten((const char*)data);
					if (err != B_OK ) io << strerror(err);
					else {
						print_message(io, getit, level);
						handled = true;
					}
				} break;
				case B_POINT_TYPE: {
					if (size == sizeof(BPoint)) {
						io << *(const BPoint*)data;
						handled = true;
					}
				} break;
				case B_RECT_TYPE: {
					if (size == sizeof(BRect)) {
						io << *(const BRect*)data;
						handled = true;
					}
				} break;
				case B_REF_TYPE: {
					entry_ref ref;
					status_t err = msg.FindRef(name, j, &ref);
					if (err != B_OK) io << strerror(err);
					else {
						io << "entry_ref(dev=" << ref.device
						   << ", dir=" << ref.directory
						   << ", name=" << ref.name << ")";
						handled = true;
					}
				} break;
				case B_RGB_COLOR_TYPE: {
					if (size == sizeof(rgb_color)) {
						io << (*(const rgb_color*)data);
						handled = true;
					}
				} break;
				case B_STRING_TYPE: {
					if (size > 0) {
						for (int32 i=0; i<size-1; i++) {
							if (((const char*)data)[i] == 0
									|| ((const char*)data)[size-1] != 0) {
								io << "string " << size << " bytes: "
								   << (BHexDump(data, size)
										.SetSingleLineCutoff(8)
										.SetPrefix(IndentForLevel(level+1)))
								   << endl;
								handled = true;
								break;
							}
						}
					}
					if (!handled) {
						io << "string(\"" << (const char*)data << "\", " << size
						   << " bytes)";
						handled = true;
					}
				} break;
				}
			}
			if (handled) io << endl;
			else {
				io << TypeToString(type, &buf) << " " << size << " bytes:"
				   << (BHexDump(data, size)
						.SetSingleLineCutoff(8)
						.SetPrefix(IndentForLevel(level+1)))
					<< endl;
			}
		}
	}
	io << IndentForLevel(level-1) << "}";
#else
	(void)msg;
	(void)level;
#endif
	return io;
}

BDataIO& operator<<(BDataIO& io, const BMessage& message)
{
	return BMessage::print_message(io, message, 0);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

const char *B_SPECIFIER_ENTRY		= "specifiers";
const char *B_PROPERTY_ENTRY		= "property";
const char *B_PROPERTY_NAME_ENTRY	= "name";
const char *_CUR_SPECIFIER_ENTRY_	= "_cur_specifier_";

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

#ifdef DEBUG_SHARED_HEAP
#define USE_MSG_CACHE 0
#else
#define USE_MSG_CACHE 1
#endif

#define MSG_CACHE_SIZE 10

BBlockCache	*BMessage::sMsgCache = NULL;

/* ----------------------------------------------------------------- */

// The new and delete operators will keep a cache of 10 BMessage blocks.
// This will improve the performance of the messaging system.

void *BMessage::operator new(size_t size)
{
#if USE_MSG_CACHE

	if (sMsgCache == NULL)
		sMsgCache = new BBlockCache(MSG_CACHE_SIZE, size, B_OBJECT_CACHE);

	return sMsgCache->Get(size);

#else
	return ::operator new(size);
#endif
}

void *BMessage::operator new(size_t size, const std::nothrow_t&)
{
#if USE_MSG_CACHE

	if (sMsgCache == NULL)
		sMsgCache = new BBlockCache(MSG_CACHE_SIZE, size, B_OBJECT_CACHE);

	return sMsgCache->Get(size);

#else
	return ::operator new(size, nothrow);
#endif
}

void *BMessage::operator new(size_t, void * arg)
{
	return arg;
}

/* ----------------------------------------------------------------- */

void BMessage::operator delete(void *ptr, size_t size)
{
#if USE_MSG_CACHE
	sMsgCache->Save(ptr, size);
#else
	::operator delete(ptr);
#endif
}

/* ----------------------------------------------------------------- */

extern "C" {

void _msg_cache_cleanup_()
{
	delete BMessage::sMsgCache;
	BMessage::sMsgCache = NULL;
}

}

/* ----------------------------------------------------------------- */

void BMessage::init_data()
{
	what = 0;
	fNext = fPrevious = NULL;
	fOriginal = NULL;
	fWhen = 0;
	fCurSpecifier = -1;
	
	fBody = NULL;

	*(message_target*)fTarget = message_target();
	
	fHasWhen = fReadOnly = fFlattenWithTarget = false;
}

/* ----------------------------------------------------------------- */
	
BMessage::BMessage()
{
	NOTE_COUNT(this, Create);
	init_data();
}

/* ----------------------------------------------------------------- */

BMessage::BMessage(uint32 pwhat)
{
	NOTE_COUNT(this, Create);
	init_data();
	what = pwhat;	
}

/* ----------------------------------------------------------------- */

BMessage::BMessage(const BMessage &msg)
{
	NOTE_COUNT(this, Create);
	START_TIMING();
	
	init_data();
	if (msg.fBody) {
		if (msg.fReadOnly) fBody = msg.fBody->Clone();
		else fBody = msg.fBody;
		fBody->Acquire();
	}
	
	what = msg.what;
	fWhen = msg.fWhen;
	fCurSpecifier = msg.fCurSpecifier;
	fHasWhen = msg.fHasWhen;
	
	NOTE_TIMING_SIZE(this, Copy, CountNames(B_ANY_TYPE));
}

/* ----------------------------------------------------------------- */

BMessage::~BMessage()
{
	if (fOriginal != (BMessage*)1) {
		make_real_empty();
		fOriginal = (BMessage*)1;
		NOTE_COUNT(this, Delete);
	} else {
		debugger("Message deleted twice");
	}
}

/* ----------------------------------------------------------------- */

BMessage &BMessage::operator=(const BMessage &msg)
{
	if (this == &msg)
		return *this;
	
	make_real_empty();
	
	if (msg.fBody) {
		if (msg.fReadOnly) fBody = msg.fBody->Clone();
		else fBody = msg.fBody;
		fBody->Acquire();
	}
	
	what = msg.what;
	fWhen = msg.fWhen;
	fCurSpecifier = msg.fCurSpecifier;
	fHasWhen = msg.fHasWhen;
	
	return *this;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::Update(const BMessage& from, bool recursive)
{
	BMessageBody::data_info info;
	data_off lastPos = 0;
	data_off pos = 0;
	
	// For each name in 'from'...
	while (from.fBody->FindData(NULL, B_ANY_TYPE, 0, &info, -1, &pos) == B_OK) {
		bool all_gone = false;
		
		// For each data item in this name...
		for( size_t i=0; i<info.count; i++ ) {
			BMessageBody::data_info data;
			data_off tmpPos = lastPos;
			if (from.fBody->FindData(NULL, B_ANY_TYPE, 0, &data, i, &tmpPos) != B_OK)
				continue;
			
			// If there still might be data under this name in the current
			// message, try to replace it with the data found in 'from'.
			if (!all_gone) {
				bool failed = false;
				
				// If requested, recursively replace nested messages.
				if (recursive && data.type == B_MESSAGE_TYPE) {
					BMessage oldMsg;
					BMessage newMsg;
					if (FindMessage(info.name, i, &oldMsg) == B_OK &&
						newMsg.Unflatten((const char*)data.data) == B_OK) {
						oldMsg.Update(newMsg, recursive);
						ReplaceMessage(info.name, i, &oldMsg);
					} else {
						failed = true;
					}
				
				// Otherwise, just overwrite the existing data.
				} else if (ReplaceData(data.name, data.type, i,
									   data.data, data.size) != B_OK) {
					failed = true;
				}
				
				// If the attempt failed, completely remove the existing
				// data and note that it is gone.
				if (failed) {
					if (i == 0) {
						RemoveName(info.name);
					} else {
						long cnt=0;
						type_code mtype = info.type;
						if( !GetInfo(info.name, &mtype, &cnt) ) {
							for( size_t k=cnt-1; k>=i; k-- ) {
								RemoveData(info.name, k);
							}
						}
					}
					all_gone = true;
				}
			}
			
			// If this name/item is not in the existing message, add
			// it in.
			if( all_gone )
				AddData(data.name, data.type, data.data, data.size);
		}
		
		if (!all_gone) {
			// Remove any remaining data entries.
			long cnt=0;
			type_code mtype;
			if( !GetInfo(info.name, &mtype, &cnt) ) {
				for( size_t k=cnt-1; k>=info.count; k-- ) {
					RemoveData(info.name, k);
				}
			}
		}
		
		lastPos = pos;
	}
	
	return B_OK;
}

status_t BMessage::FillIn(const BMessage& from, bool recursive)
{
	BMessageBody::data_info info;
	data_off lastPos = 0;
	data_off pos = 0;
	
	// For each name in 'from'...
	while (from.fBody->FindData(NULL, B_ANY_TYPE, 0, &info, -1, &pos) == B_OK) {
		type_code origType;
		int32 origCount;
		if (GetInfo(info.name, &origType, &origCount) != B_OK)
			origCount = 0;
		
		// If both the existing and 'from' data is a message, and recursion
		// has been requested, then extract and fill in the message.
		if (recursive && info.type == B_MESSAGE_TYPE && origType == B_MESSAGE_TYPE) {
			const int32 N = origCount < (int32)(info.count)
						  ? origCount : (int32)(info.count);
			for (int32 i=0; i<N; i++) {
				BMessageBody::data_info data;
				data_off tmpPos = lastPos;
				if (from.fBody->FindData(NULL, B_ANY_TYPE, 0, &data, i, &tmpPos) != B_OK)
					continue;
				BMessage oldMsg;
				BMessage newMsg;
				if (FindMessage(info.name, i, &oldMsg) == B_OK &&
					newMsg.Unflatten((const char*)data.data) == B_OK) {
					oldMsg.FillIn(newMsg, recursive);
					ReplaceMessage(info.name, i, &oldMsg);
				}
			}
		}
		
		// For all data in 'from' that is not in our existing message,
		// copy it in.  In the case of mis-matched types, we leave the
		// existing data alone.
		for (int32 i=origCount; i<(int32)info.count; i++) {
			BMessageBody::data_info data;
			data_off tmpPos = lastPos;
			if (from.fBody->FindData(NULL, B_ANY_TYPE, 0, &data, i, &tmpPos) != B_OK)
				continue;
			if (AddData(data.name, data.type, data.data, data.size) != B_OK)
				break;
		}
		
		lastPos = pos;
	}
	
	return B_OK;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

inline BMessageBody* BMessage::edit_body()
{
	BMessageBody* body = fBody->Edit();
	if (body) fBody = body;
	return body;
}

inline BMessageBody* BMessage::reset_body()
{
	BMessageBody* body = fBody->Reset();
	fBody = body;
	return body;
}

inline void BMessage::get_args(BPrivate::header_args* into) const
{
	into->init(what, fCurSpecifier, fWhen, fHasWhen);
}

inline void BMessage::get_args(BPrivate::header_args* into, bigtime_t when) const
{
	into->init(what, fCurSpecifier, when, true);
}

inline void BMessage::set_args(const BPrivate::header_args* from)
{
	what = from->what;
	fCurSpecifier = from->cur_specifier;
	fWhen = from->when;
	fHasWhen = from->has_when;
}
		
inline void BMessage::reply_if_needed()
{
	if (IsReplyRequested()) {
		BMessage m(B_NO_REPLY);
		SendReply(&m);
	}
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::GetInfo(const char *name, type_code *type,
	int32 *count) const
{
	BMessageBody::data_info info;
	status_t res = fBody->FindData(name, &info);
	if (res != B_OK) {
		if (type) *type = 0;
		if (count) *count = 0;
		return res;
	}
	
	if (type) *type = info.type;
	if (count) *count = info.count;
	
	return B_OK;
}

/* ----------------------------------------------------------------- */

status_t BMessage::GetInfo(const char *name, type_code *type,
	bool *is_fixed_size) const
{
	BMessageBody::data_info info;
	status_t res = fBody->FindData(name, &info);
	if (res != B_OK) {
		if (type) *type = 0;
		if (is_fixed_size) *is_fixed_size = false;
		return res;
	}
	
	if (type) *type = info.type;
	if (is_fixed_size) *is_fixed_size = info.size != NULL_SIZE;
	
	return B_OK;
}

/* ----------------------------------------------------------------- */

status_t BMessage::GetInfo(type_code typeRequested, int32 which, const char **name, 
	type_code *typeReturned, int32 *count) const
{
	BMessageBody::data_info info;
	status_t res = fBody->FindData(0, typeRequested, which, &info);
	if (res != B_OK) {
		if (name) *name = 0;
		if (typeReturned) *typeReturned = B_ANY_TYPE;
		if (count) *count = 0;
		return res;
	}
	
	if (name) *name = info.name;
	if (typeReturned) *typeReturned = info.type;
	if (count) *count = info.count;
	
	return B_OK;
}

/* ----------------------------------------------------------------- */

status_t BMessage::GetNextName(void **cookie, const char **outName,
							  type_code *outType, int32 *outCount) const
{
	BMessageBody::data_info info;
	status_t res = fBody->FindData(NULL, B_ANY_TYPE, 0, &info, -1,
								   (data_off*)cookie);
	if (res != B_OK) {
		*outName = NULL;
		if (outType) *outType = B_ANY_TYPE;
		if (outCount) *outCount = 0;
		return res;
	}
	
	*outName = info.name;
	if (outType) *outType = info.type;
	if (outCount) *outCount = info.count;
	
	return B_OK;
}

/* ----------------------------------------------------------------- */

int32	BMessage::CountNames(type_code type) const
{
	return fBody->CountNames(type);
}

/* ----------------------------------------------------------------- */

bool	BMessage::IsEmpty() const
{
	return (fBody == NULL) || fBody->IsEmpty();
}

/* ----------------------------------------------------------------- */

bool	BMessage::IsSystem() const
{
	bool	result = true;

	if ((what>>24) != '_')
		result = false;

	else if ((((what>>16)&0xFF) < 'A') || (((what>>16)&0xFF) > 'Z'))
		result = false;

	else if (((what>>8)&0xFF) < 'A' || ((what>>8)&0xFF) > 'Z')
		result = false;

	else if ((what&0xFF) < 'A' || (what&0xFF) > 'Z')
		result = false;

	return result;
}

/* ----------------------------------------------------------------- */

bool BMessage::IsReply() const
{
	return ( (((message_target*)fTarget)->flags&(MTF_DELIVERED|MTF_IS_REPLY))
			== (MTF_DELIVERED|MTF_IS_REPLY) );
}

/* ----------------------------------------------------------------- */

void BMessage::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

/* ----------------------------------------------------------------- */

status_t BMessage::Rename(const char *old_name, const char *new_name)
{
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	// Should we deal with renaming "when"??  Well, we -should-...
	// but is it worth it?
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	status_t e;
	fBody = body->Rename(old_name, new_name, &e);
	
	return e;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

bool BMessage::WasDelivered() const
{
	return ((((message_target*)fTarget)->flags&MTF_DELIVERED) ? true : false);
}

/* ----------------------------------------------------------------- */

bool BMessage::IsReplyRequested() const
{
	// If this message does not have a reply target or a reply has
	// already been sent, nothing to return.
	if ( (((message_target*)fTarget)->flags&(MTF_DELIVERED|MTF_REPLY_SENT)) != MTF_DELIVERED)
		return false;
		
	// If another thread is not waiting for the message and the sender
	// did not request a reply, nothing requested.
	return ( (((message_target*)fTarget)->flags&(MTF_SOURCE_IS_WAITING|MTF_REPLY_REQUIRED))
			!= 0 );
}

/* ----------------------------------------------------------------- */

bool BMessage::IsSourceWaiting() const
{
	return ( (((message_target*)fTarget)->flags&(MTF_DELIVERED|MTF_SOURCE_IS_WAITING|MTF_REPLY_SENT))
			== (MTF_DELIVERED|MTF_SOURCE_IS_WAITING) );
}

/* ----------------------------------------------------------------- */

bool BMessage::IsSourceRemote() const
{
	if (!(((message_target*)fTarget)->flags&MTF_DELIVERED))
		return false;

	return ((message_target*)fTarget)->reply_team != _find_cur_team_id_();
}

/* ----------------------------------------------------------------- */

bool BMessage::CompareDestination(const BMessenger& test) const
{
	if (!(((message_target*)fTarget)->flags&MTF_DELIVERED))
		return false;
	
	if (test.fTeam != _find_cur_team_id_()) return false;
	
	const message_target& target(*((message_target*)fTarget));
	if (test.fHandlerToken != target.target) return false;
	if (test.fPreferredTarget != ((target.flags&MTF_PREFERRED_TARGET) ? true : false))
		return false;
	
	return true;
}

/* ----------------------------------------------------------------- */

BMessenger BMessage::ReturnAddress() const
{
	if (!(((message_target*)fTarget)->flags&MTF_DELIVERED))
		return BMessenger();
	
	const message_target& target(*((message_target*)fTarget));
	
	return BMessenger(target.reply_team, target.reply_port, target.reply_target,
					  target.flags&MTF_PREFERRED_REPLY);
}

/* ----------------------------------------------------------------- */

const BMessage *BMessage::Previous() const
{
	if (!IsReply())
		return NULL;
	
	if (fOriginal)
		return fOriginal;

	/*
	 If 'this' is a reply to an async message, then a copy of the original
	 message, that caused the reply can be found (as data) in the
	 "_previous_" data field.
	*/
	fOriginal = new BMessage();
	if (FindMessage("_previous_", fOriginal) != B_OK) {
		delete fOriginal;
		fOriginal = NULL;
	}

	return fOriginal;
}

/* ----------------------------------------------------------------- */

bool BMessage::WasDropped() const
{
	return HasPoint("_drop_point_");
}

/* ----------------------------------------------------------------- */

BPoint BMessage::DropPoint(BPoint *offset) const
{
	if (offset)
		*offset = FindPoint("_drop_offset_");
	return FindPoint("_drop_point_");
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

bool BMessage::HasWhen() const
{
	if (fHasWhen) return true;
	return HasInt64("when");
}

bigtime_t BMessage::When() const
{
	if (fHasWhen) return fWhen;
	else return FindInt64("when");
}

void BMessage::SetWhen(bigtime_t time)
{
	if (!fHasWhen && HasInt64("when"))
		RemoveName("when");
	fWhen = time;
	fHasWhen = true;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

/* ----------------------------------------------------------------- */

status_t BMessage::SendReply(const BMessage& the_reply,
	const BMessenger& reply_to, uint32 flags, bigtime_t timeout)
{
	BMessenger to(ReturnAddress());
	if (!to.IsValid()) {
		return B_BAD_REPLY;
	}
	
	message_target&	target = *(message_target*)fTarget;
	
	status_t		error;
	
	if ( (target.flags&MTF_SOURCE_IS_WAITING) != 0 ) {
		if ( (target.flags&MTF_REPLY_SENT) != 0 ) {
			return B_DUPLICATE_REPLY;
		}

//+		SERIAL_PRINT(("replying (sync) to msg \"%.4s\", to team %d\n",
//+			(char*) &what, to.fTeam));
		error = to.SendMessage(the_reply, reply_to, flags|MTF_IS_REPLY, timeout);
		
		if (error) {
			// Some error. Perhaps the target is dead. Try to return ownership
			// of the tmp_port, or delete it if that fails.
			if (set_port_owner(to.fPort, to.fTeam) == B_BAD_TEAM_ID)
				delete_port(to.fPort);
		}
	} else {
		/*
		 The message was 'sent' by someone, but they aren't waiting
		 for a synchronouse reply.  Flatten the original message ('this')
		 and put a pointer to the flattened data in the
		 'reply' message. In this way the originating message will
		 get a copy of the original message it sent.
		*/
		
		if (&the_reply != this)
			const_cast<BMessage&>(the_reply).AddMessage("_previous_", this);
		error = to.SendMessage(the_reply, reply_to, flags|MTF_IS_REPLY, timeout);
		if (&the_reply != this)
			const_cast<BMessage&>(the_reply).RemoveData("_previous_");
	}

	if (error == B_OK) {
		target.flags |= MTF_REPLY_SENT;
	}
	
	return error;
}

status_t BMessage::SendReply(uint32 command)
{
	BMessage m(command);
	return SendReply(m, BMessenger());
}

status_t BMessage::SendReply(uint32 command, const BMessenger& reply_to)
{
	BMessage m(command);
	return SendReply(m, reply_to);
}

/* ----------------------------------------------------------------- */

status_t BMessage::SendReply(const BMessage& the_reply, BMessage* reply_to_reply,
	uint32 flags, bigtime_t send_timeout, bigtime_t reply_timeout)
{
	/*
	 The 'this' message is being replied to with 'the_reply'. 
	 A synchronous response is expected.
	*/
	BMessenger to(ReturnAddress());
	if (!to.IsValid()) {
		return B_BAD_REPLY;
	}
	
	message_target&	target = *(message_target*)fTarget;
	
	status_t		error;
	
	if ( (target.flags&MTF_SOURCE_IS_WAITING) != 0 ) {
		if ( (target.flags&MTF_REPLY_SENT) != 0 ) {
			return B_DUPLICATE_REPLY;
		}

		ASSERT(to.fHandlerToken == -1);		// sync replies don't have tokens
		error = to.SendMessage(the_reply, reply_to_reply,
							   flags|MTF_IS_REPLY, send_timeout, reply_timeout);

		if (error) {
			// Some error. Perhaps the target is dead. Try to return ownership
			// of the tmp_port, or delete it if that fails.
			if (set_port_owner(to.fPort, to.fTeam) == B_BAD_TEAM_ID)
				delete_port(to.fPort);
		}
	} else {
		/*
		 The message was 'sent' by someone, but they aren't waiting
		 for a synchronouse reply.  Flatten the original message ('this')
		 and put a pointer to the flattened data in the
		 'reply' message. In this way the originating message will
		 get a copy of the original message it sent.
		*/
		
		if (&the_reply != this)
			const_cast<BMessage&>(the_reply).AddMessage("_previous_", this);
		error = to.SendMessage(the_reply, reply_to_reply,
							   flags|MTF_IS_REPLY, send_timeout, reply_timeout);
		if (&the_reply != this)
			const_cast<BMessage&>(the_reply).RemoveData("_previous_");
	}

	if (error == B_OK) {
		target.flags |= MTF_REPLY_SENT;
	}
	
	return error;
}

/* ----------------------------------------------------------------- */

status_t BMessage::SendReply(uint32 command, BMessage* reply_to_reply)
{
	BMessage m(command);
	return SendReply(m, reply_to_reply);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::SendReply(uint32 command, BHandler *reply_to)
{
	BMessage m(command);
	return SendReply(&m, reply_to);
}

status_t BMessage::SendReply(BMessage *the_reply, BHandler *reply_to,
	bigtime_t timeout)
{
	/*
	 The 'this' message is being replied to with 'the_reply'. Any 
	 replies to the reply will be sent to 'reply_to'
	*/

	BMessenger	reply_mess(reply_to);

	return SendReply(*the_reply, reply_mess, B_TIMEOUT, timeout);
}

status_t BMessage::SendReply(BMessage *the_reply, BMessenger reply_to,
	bigtime_t timeout)
{
	return SendReply(*the_reply, reply_to, B_TIMEOUT, timeout);
}

status_t BMessage::SendReply(BMessage *the_reply, BMessage *reply_to_reply,
	bigtime_t send_timeout, bigtime_t reply_timeout)
{
	return SendReply(*the_reply, reply_to_reply,
					 B_TIMEOUT, send_timeout, reply_timeout);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

ssize_t	BMessage::FlattenedSize() const
{
	header_args args;
	get_args(&args);
	return fBody->CalcFlatSize(&args, fFlattenWithTarget ? true : false);
}

/* ---------------------------------------------------------------- */

status_t BMessage::Flatten(BDataIO *stream, ssize_t *psize) const
{
	if (fBody->CountAtoms()) {
		debugger("Can't flatten a message containing atoms");
	}
	
	header_args args;
	get_args(&args);
	return fBody->Flatten(&args, stream, psize,
						  fFlattenWithTarget ? ((const message_target*)fTarget) : NULL);
}

/* ----------------------------------------------------------------- */

status_t BMessage::Flatten(char *result, ssize_t size) const
{
	if (fBody->CountAtoms()) {
		debugger("Can't flatten a message containing atoms");
	}
	
	header_args args;
	get_args(&args);
	return fBody->Flatten(&args, result, size,
						  fFlattenWithTarget ? ((const message_target*)fTarget) : NULL);
}

/* ----------------------------------------------------------------- */

status_t BMessage::flatten_no_check(char *result, ssize_t size) const
{
	header_args args;
	get_args(&args);
	return fBody->Flatten(&args, result, size,
						  fFlattenWithTarget ? ((const message_target*)fTarget) : NULL);
}

/* ----------------------------------------------------------------- */

ssize_t	BMessage::FlattenedSize(message_version format) const
{
	if (fBody->CountAtoms()) {
		debugger("Can't flatten a message containing atoms");
	}
	
	header_args args;
	get_args(&args);
	
	switch (format) {
		case B_MESSAGE_VERSION_1:
			return flatten_old_message(NULL, fBody, &args);
		
		case B_MESSAGE_VERSION_2:
		case B_MESSAGE_VERSION_ANY:
			return fBody->CalcFlatSize(&args, fFlattenWithTarget ? true : false);
	}
	
	return B_UNSUPPORTED;
}

/* ---------------------------------------------------------------- */

status_t BMessage::Flatten(message_version format, BDataIO *stream, ssize_t *psize) const
{
	if (fBody->CountAtoms()) {
		debugger("Can't flatten a message containing atoms");
	}
	
	header_args args;
	get_args(&args);
	const message_target* target =
		fFlattenWithTarget ? ((const message_target*)fTarget) : NULL;
	
	switch (format) {
		case B_MESSAGE_VERSION_1: {
			ssize_t size = flatten_old_message(NULL, fBody, &args);
			if (size <= 0) return size;
			uint8* data = (uint8*)malloc(size);
			if (!data) return B_NO_MEMORY;
			size = flatten_old_message(data, fBody, &args);
			if (size < 0) {
				free(data);
				return size;
			}
			ssize_t written = stream->Write(data, size);
			free(data);
			if (written < B_OK) return written;
			if (written != size) return B_IO_ERROR;
			if (psize) *psize = written;
			return B_OK;
		} break;
		
		case B_MESSAGE_VERSION_2:
		case B_MESSAGE_VERSION_ANY:
			return fBody->Flatten(&args, stream, psize, target);
	}
	
	return B_UNSUPPORTED;
}

/* ----------------------------------------------------------------- */

status_t BMessage::Flatten(message_version format, char *result, ssize_t size) const
{
	if (fBody->CountAtoms()) {
		debugger("Can't flatten a message containing atoms");
	}
	
	header_args args;
	get_args(&args);
	const message_target* target =
		fFlattenWithTarget ? ((const message_target*)fTarget) : NULL;
	
	switch (format) {
		case B_MESSAGE_VERSION_1: {
			status_t error = flatten_old_message((uint8*)result, fBody, &args);
			if (error < B_OK) return error;
			return B_OK;
		} break;
		
		case B_MESSAGE_VERSION_2:
		case B_MESSAGE_VERSION_ANY:
			return fBody->Flatten(&args, result, size, target);
	}
	
	return B_UNSUPPORTED;
}

/* ----------------------------------------------------------------- */

status_t BMessage::Unflatten(BDataIO *stream)
{
	START_TIMING();
	
	// The first thing we do is try to read the initial four-byte
	// signature, so that if it doesn't exist we can fail without
	// changing the existing message.  It's done this way for (some)
	// backwards compatibility with the previous behaviour of BMessage.
	uint32 sig;
	ssize_t err = stream->Read(&sig, sizeof(sig));
	if (err < (ssize_t)sizeof(sig))
		return (err < B_OK ? err : B_NOT_A_MESSAGE);
	if (sig != MESSAGE_SIGNATURE && sig != SWAPPED_MESSAGE_SIGNATURE &&
			sig != 'FOB1' && sig != '1BOF')
		return B_NOT_A_MESSAGE;
	
	BMessageBody* body = 0;
	if (fBody) body = reset_body();
	
	*(message_target*)fTarget = message_target();
	header_args args;
	args.init();
	
	err = B_OK;
	fBody = body->Unflatten(stream, sig, &args, (message_target*)fTarget, &err);
	
	if (err != B_OK) {
		// Return to consistent, empty state; this will also send a
		// required reply if we managed to get that much info from
		// the stream.
		make_real_empty();
		return err;
	}
	
	set_args(&args);
	
	NOTE_TIMING_SIZE(this, UnflattenStream, CountNames(B_ANY_TYPE));
	
	return err;
}

/* ----------------------------------------------------------------- */

status_t BMessage::Unflatten(const char *ptr)
{
	START_TIMING();
	
	BMessageBody* body = 0;
	if (fBody) body = reset_body();
		
	*(message_target*)fTarget = message_target();
	header_args args;
	args.init();
	
	status_t err = B_OK;
	fBody = body->Unflatten(ptr, 0x7fffffff, &args, (message_target*)fTarget, &err);
	
	if (err != B_OK) {
		// Return to consistent, empty state; this will also send a
		// required reply if we managed to get that much info from
		// the stream.
		make_real_empty();
		return err;
	}
	
	set_args(&args);
	
	NOTE_TIMING_SIZE(this, Unflatten, CountNames(B_ANY_TYPE));
	
	return err;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::WritePort(port_id port, int32 code, uint32 flags,
							  bigtime_t timeout) const
{
	header_args args;
	get_args(&args);
	return fBody->WritePort(port, code, &args,
							fFlattenWithTarget ? ((const message_target*)fTarget) : NULL,
							flags, timeout);
}

/* ----------------------------------------------------------------- */

status_t BMessage::ReadPort(port_id port, ssize_t size, int32* outCode,
							 uint32 flags, bigtime_t timeout)
{
	BMessageBody* body = 0;
	if (fBody) body = reset_body();
		
	*(message_target*)fTarget = message_target();
	header_args args;
	args.init();
	
	status_t err = B_OK;
	fBody = body->ReadPort(port, size, outCode, &args, (message_target*)fTarget,
						   &err, flags, timeout);
	
	set_args(&args);
	
	if (err != B_OK && err != B_NOT_A_MESSAGE) {
		// Return to consistent, empty state; this will also send a
		// required reply if we managed to get that munch info from
		// the stream.
		// Note that we don't make empty for B_NOT_A_MESSAGE errors,
		// because those may return raw port data in a valid message.
		make_real_empty();
	}
	
	return err;
}

/* ----------------------------------------------------------------- */

size_t BMessage::RawPortSize() const
{
	return 0;
}

const void*	BMessage::RawPortData() const
{
	return NULL;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddSpecifier(const BMessage *spec)
{
	status_t	err;
	err = AddMessage(B_SPECIFIER_ENTRY, spec);
	if (!err) {
		BMessageBody* body = edit_body();
		if (!body)
			return B_NO_MEMORY;
		
		fCurSpecifier++;
		if (!body->HasSpecifiers()) {
			// This is the first specifier
			ASSERT(fCurSpecifier == 0);
			body->SetHasSpecifiers(true);
		}
	}
	return err;
}

/* ----------------------------------------------------------------- */

status_t BMessage::AddSpecifier(const char *property)
{
	status_t	err;
	BMessage	msg;

	msg.what = B_DIRECT_SPECIFIER;
	if ((err = msg.AddString(B_PROPERTY_ENTRY, property)) != B_OK)
		return err;
	return AddSpecifier(&msg);
}

/* ----------------------------------------------------------------- */

status_t BMessage::AddSpecifier(const char *property, int32 index)
{
	status_t	err;
	BMessage	msg;

	msg.what = B_INDEX_SPECIFIER;
	if ((err = msg.AddString(B_PROPERTY_ENTRY, property)) != B_OK)
		return err;
	if ((err = msg.AddInt32("index", index)) != B_OK)
		return err;
	return AddSpecifier(&msg);
}

/* ----------------------------------------------------------------- */

status_t BMessage::AddSpecifier(const char *property, int32 index,
	int32 range)
{
	status_t	err;
	BMessage	msg;

	if (range < 0)
		return B_BAD_VALUE;

	msg.what = B_RANGE_SPECIFIER;
	if ((err = msg.AddString(B_PROPERTY_ENTRY, property)) != B_OK)
		return err;
	if ((err = msg.AddInt32("index", index)) != B_OK)
		return err;
	if ((err = msg.AddInt32("range", range)) != B_OK)
		return err;
	return AddSpecifier(&msg);
}

/* ----------------------------------------------------------------- */

status_t BMessage::AddSpecifier(const char *property, const char *item_name)
{
	status_t	err;
	BMessage	msg;

	msg.what = B_NAME_SPECIFIER;
	if ((err = msg.AddString(B_PROPERTY_ENTRY, property)) != B_OK)
		return err;
	if ((err = msg.AddString(B_PROPERTY_NAME_ENTRY, item_name)) != B_OK)
		return err;

	return AddSpecifier(&msg);
}

/* ----------------------------------------------------------------- */

status_t BMessage::SetCurrentSpecifier(int32 index)
{
	status_t	err;
	type_code	type;
	int32		c;

	if ((err = GetInfo(B_SPECIFIER_ENTRY, &type, &c)) != B_OK)
		return err;

	if (index < 0 || index >= c)
		return B_BAD_INDEX;

	fCurSpecifier = index;
	
	return B_OK;
}

/* ----------------------------------------------------------------- */

status_t BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier,
	 int32 *form, const char **property) const
{
	status_t err;

	*index = fCurSpecifier;

	if (*index < 0)
		return B_BAD_SCRIPT_SYNTAX;

	if (!specifier)
		return B_OK;

	err = FindMessage(B_SPECIFIER_ENTRY, *index, specifier);
	if (err)
		return B_BAD_SCRIPT_SYNTAX;

	if (form)
		*form = specifier->what;
	if (property) {
		err = specifier->FindString(B_PROPERTY_ENTRY, property);
		if (err)
			err = B_BAD_SCRIPT_SYNTAX;
	}

	return err;
}

/* ----------------------------------------------------------------- */

bool BMessage::HasSpecifiers() const
{
	return fBody->HasSpecifiers();
}

/* ----------------------------------------------------------------- */

status_t BMessage::PopSpecifier()
{
	// can't start popping specifiers until the msg is received.

	if (!WasDelivered())
		return B_BAD_VALUE;

	if (fCurSpecifier < 0)
		return B_BAD_VALUE;

	fCurSpecifier--;
	return B_OK;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddRect(const char *name, BRect r)
{
	return fast_add_data(name, B_RECT_TYPE, &r, sizeof(BRect));
} 

status_t BMessage::FindRect(const char *name, BRect *r) const
{
	return FindRect(name, 0, r);
}

status_t BMessage::FindRect(const char *name, int32 index, BRect *r) const
{
	const status_t e = fBody->CopyData(name, B_RECT_TYPE, index, r, sizeof(BRect));
	if (e != B_OK) *r = BRect();
	return e;
}

BRect BMessage::FindRect(const char *name, int32 index) const
{
	BRect f;
	FindRect(name, index, &f);
	return f;
}

bool BMessage::HasRect(const char *name, int32 index) const
{
	return(HasData(name, B_RECT_TYPE, index));
}

status_t BMessage::ReplaceRect(const char *name, BRect new_rect)
{
	return fast_replace_data(name, B_RECT_TYPE, 0, &new_rect, sizeof(BRect));
}

status_t BMessage::ReplaceRect(const char *name, int32 index, BRect new_rect)
{
	return fast_replace_data(name, B_RECT_TYPE, index, &new_rect, sizeof(BRect));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddPoint(const char *name, BPoint pt)
{
	return fast_add_data(name, B_POINT_TYPE, &pt, sizeof(BPoint));
} 

status_t BMessage::FindPoint(const char *name, BPoint *pt) const
{
	return FindPoint(name, 0, pt);
}

status_t BMessage::FindPoint(const char *name, int32 index, BPoint *pt) const
{
	const status_t e = fBody->CopyData(name, B_POINT_TYPE, index, pt, sizeof(BPoint));
	if (e != B_OK) *pt = BPoint(0, 0);
	return e;
}

BPoint BMessage::FindPoint(const char *name, int32 index) const
{
	BPoint f;
	FindPoint(name, index, &f);
	return f;
}

bool BMessage::HasPoint(const char *name, int32 index) const
{
	return(HasData(name, B_POINT_TYPE, index));
}

status_t BMessage::ReplacePoint(const char *name, BPoint new_point)
{
	return fast_replace_data(name, B_POINT_TYPE, 0, &new_point, sizeof(BPoint));
}

status_t BMessage::ReplacePoint(const char *name, int32 index, BPoint new_point)
{
	return fast_replace_data(name, B_POINT_TYPE, index, &new_point, sizeof(BPoint));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddString(const char *name, const char *str)
{
	return fast_add_data(name, B_STRING_TYPE, str, strlen(str) + 1, false);
} 

status_t BMessage::FindString(const char *name, const char **str) const
{
	return FindString(name, 0, str);
}

status_t BMessage::FindString(const char *name, int32 index, const char **str) const
{
	ssize_t size;
	return fast_find_data(name, B_STRING_TYPE, index, (const void**)str, &size);
}

const char* BMessage::FindString(const char *name, int32 index) const
{
	const char *f;
	FindString(name, index, &f);
	return f;
}

bool BMessage::HasString(const char *name, int32 index) const
{
	return(HasData(name, B_STRING_TYPE, index));
}

status_t BMessage::ReplaceString(const char *name, const char *new_string)
{
	return fast_replace_data(name, B_STRING_TYPE, 0, (void *) new_string, strlen(new_string) + 1);
}

status_t BMessage::ReplaceString(const char *name, int32 index,
	const char *new_string)
{
	return fast_replace_data(name, B_STRING_TYPE, index, (void *) new_string, strlen(new_string) + 1);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddString(const char *name, const BString& str)
{
	return fast_add_data(name, B_STRING_TYPE, str.String(), str.Length() + 1, false);
} 

status_t BMessage::FindString(const char *name, BString *str) const
{
	return FindString(name, 0, str);
}

status_t BMessage::FindString(const char *name, int32 index, BString *str) const
{
	const void* data;
	ssize_t size;
	const status_t e = fast_find_data(name, B_STRING_TYPE, index, &data, &size);
	if (e != B_OK || size <= 1) {
		*str = "";
		return e;
	}
	
	char* c = str->LockBuffer(size-1);
	if (!c) {
		str->UnlockBuffer();
		return B_NO_MEMORY;
	}
	
	memcpy(c, data, size-1);
	
	str->UnlockBuffer(size-1);
	
	return B_OK;
}

status_t BMessage::ReplaceString(const char *name, const BString& new_string)
{
	return fast_replace_data(name, B_STRING_TYPE, 0,
					   new_string.String(), new_string.Length() + 1);
}

status_t BMessage::ReplaceString(const char *name, int32 index,
	const BString& new_string)
{
	return fast_replace_data(name, B_STRING_TYPE, index,
					   new_string.String(), new_string.Length() + 1);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddInt8(const char *name, int8 val)
{
	return fast_add_data(name, B_INT8_TYPE, &val, sizeof(int8));
} 

status_t BMessage::FindInt8(const char *name, int8 *l) const
{
	return FindInt8(name, 0, l);
}

status_t BMessage::FindInt8(const char *name, int32 index, int8 *l) const
{
	const status_t e = fBody->CopyData(name, B_INT8_TYPE, index, l, sizeof(int8));
	if (e != B_OK) *l = 0;
	return e;
}

int8 BMessage::FindInt8(const char *name, int32 index) const
{
	int8	l;
	FindInt8(name, index, &l);
	return l;
}

bool BMessage::HasInt8(const char *name, int32 index) const
{
	return(HasData(name, B_INT8_TYPE, index));
}

status_t BMessage::ReplaceInt8(const char *name, int8 val)
{
	return fast_replace_data(name, B_INT8_TYPE, 0, &val, sizeof(int8));
}

status_t BMessage::ReplaceInt8(const char *name, int32 index, int8 val)
{
	return fast_replace_data(name, B_INT8_TYPE, index, &val, sizeof(int8));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddInt16(const char *name, int16 val)
{
	return fast_add_data(name, B_INT16_TYPE, &val, sizeof(int16));
} 

status_t BMessage::FindInt16(const char *name, int16 *l) const
{
	return FindInt16(name, 0, l);
}

status_t BMessage::FindInt16(const char *name, int32 index, int16 *l) const
{
	const status_t e = fBody->CopyData(name, B_INT16_TYPE, index, l, sizeof(int16));
	if (e != B_OK) *l = 0;
	return e;
}

int16 BMessage::FindInt16(const char *name, int32 index) const
{
	int16	l;
	FindInt16(name, index, &l);
	return l;
}

bool BMessage::HasInt16(const char *name, int32 index) const
{
	return(HasData(name, B_INT16_TYPE, index));
}

status_t BMessage::ReplaceInt16(const char *name, int16 val)
{
	return fast_replace_data(name, B_INT16_TYPE, 0, &val, sizeof(int16));
}

status_t BMessage::ReplaceInt16(const char *name, int32 index, int16 val)
{
	return fast_replace_data(name, B_INT16_TYPE, index, &val, sizeof(int16));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddInt32(const char *name, int32 val)
{
	return fast_add_data(name, B_INT32_TYPE, &val, sizeof(int32));
} 

status_t BMessage::FindInt32(const char *name, int32 *l) const
{
	return FindInt32(name, 0, l);
}

status_t BMessage::FindInt32(const char *name, int32 index, int32 *l) const
{
	const status_t e = fBody->CopyData(name, B_INT32_TYPE, index, l, sizeof(int32));
	if (e != B_OK) *l = 0;
	return e;
}

int32 BMessage::FindInt32(const char *name, int32 index) const
{
	int32	l;
	FindInt32(name, index, &l);
	return l;
}

bool BMessage::HasInt32(const char *name, int32 index) const
{
	return(HasData(name, B_INT32_TYPE, index));
}

status_t BMessage::ReplaceInt32(const char *name, int32 val)
{
	return fast_replace_data(name, B_INT32_TYPE, 0, &val, sizeof(int32));
}

status_t BMessage::ReplaceInt32(const char *name, int32 index, int32 val)
{
	return fast_replace_data(name, B_INT32_TYPE, index, &val, sizeof(int32));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddInt64(const char *name, int64 val)
{
	return AddData(name, B_INT64_TYPE, &val, sizeof(int64));
} 

status_t BMessage::FindInt64(const char *name, int64 *l) const
{
	return FindInt64(name, 0, l);
}

status_t BMessage::FindInt64(const char *name, int32 index, int64 *l) const
{
	if (index == 0 && fHasWhen && strcmp(name, "when") == 0) {
		*l = fWhen;
		return B_OK;
	}
	
	const status_t e = fBody->CopyData(name, B_INT64_TYPE, index, l, sizeof(int64));
	if (e != B_OK) *l = 0;
	return e;
}

int64 BMessage::FindInt64(const char *name, int32 index) const
{
	int64	l;
	FindInt64(name, index, &l);
	return l;
}

bool BMessage::HasInt64(const char *name, int32 index) const
{
	return(HasData(name, B_INT64_TYPE, index));
}

status_t BMessage::ReplaceInt64(const char *name, int64 val)
{
	return ReplaceData(name, B_INT64_TYPE, 0, &val, sizeof(int64));
}

status_t BMessage::ReplaceInt64(const char *name, int32 index, int64 val)
{
	return ReplaceData(name, B_INT64_TYPE, index, &val, sizeof(int64));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddBool(const char *name, bool a_boolean)
{
	return fast_add_data(name, B_BOOL_TYPE, &a_boolean, sizeof(bool));
} 

status_t BMessage::FindBool(const char *name, bool *b) const
{
	return FindBool(name, 0, b);
}

status_t BMessage::FindBool(const char *name, int32 index, bool *b) const
{
	const status_t e = fBody->CopyData(name, B_BOOL_TYPE, index, b, sizeof(bool));
	if (e != B_OK) *b = false;
	return e;
}

bool BMessage::FindBool(const char *name, int32 index) const
{
	bool f;
	FindBool(name, index, &f);
	return f;
}

bool BMessage::HasBool(const char *name, int32 index) const
{
	return(HasData(name, B_BOOL_TYPE, index));
}

status_t BMessage::ReplaceBool(const char *name, bool a_bool)
{
	return fast_replace_data(name, B_BOOL_TYPE, 0, &a_bool, sizeof(a_bool));
}

status_t BMessage::ReplaceBool(const char *name, int32 index, bool a_bool)
{
	return fast_replace_data(name, B_BOOL_TYPE, index, &a_bool, sizeof(a_bool));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddFloat(const char *name, float a_float)
{
	return fast_add_data(name, B_FLOAT_TYPE, &a_float, sizeof(float));
} 

status_t BMessage::FindFloat(const char *name, float *f) const
{
	return FindFloat(name, 0, f);
}

status_t BMessage::FindFloat(const char *name, int32 index, float *f) const
{
	const status_t e = fBody->CopyData(name, B_FLOAT_TYPE, index, f, sizeof(float));
	if (e != B_OK) *f = 0.0f;
	return e;
}

float BMessage::FindFloat(const char *name, int32 index) const
{
	float f;
	FindFloat(name, index, &f);
	return f;
}

bool BMessage::HasFloat(const char *name, int32 index) const
{
	return(HasData(name, B_FLOAT_TYPE, index));
}

status_t BMessage::ReplaceFloat(const char *name, float new_float)
{
	return fast_replace_data(name, B_FLOAT_TYPE, 0, &new_float, sizeof(float));
}

status_t BMessage::ReplaceFloat(const char *name, int32 index, float new_float)
{
	return fast_replace_data(name, B_FLOAT_TYPE, index, &new_float, sizeof(float));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddDouble(const char *name, double a_double)
{
	return fast_add_data(name, B_DOUBLE_TYPE, &a_double, sizeof(double));
} 

status_t BMessage::FindDouble(const char *name, double *d) const
{
	return FindDouble(name, 0, d);
}

status_t BMessage::FindDouble(const char *name, int32 index, double *d) const
{
	const status_t e = fBody->CopyData(name, B_DOUBLE_TYPE, index, d, sizeof(double));
	if (e != B_OK) *d = 0.0;
	return e;
}

double BMessage::FindDouble(const char *name, int32 index) const
{
	double f;
	FindDouble(name, index, &f);
	return f;
}

bool BMessage::HasDouble(const char *name, int32 index) const
{
	return(HasData(name, B_DOUBLE_TYPE, index));
}

status_t BMessage::ReplaceDouble(const char *name, double new_double)
{
	return fast_replace_data(name, B_DOUBLE_TYPE, 0, &new_double, sizeof(double));
}

status_t BMessage::ReplaceDouble(const char *name, int32 index, double new_double)
{
	return fast_replace_data(name, B_DOUBLE_TYPE, index, &new_double, sizeof(double));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddRGBColor(const char *name, rgb_color a_color,
								type_code type)
{
	return fast_add_data(name, type, &a_color, sizeof(rgb_color));
} 

status_t BMessage::FindRGBColor(const char *name, rgb_color *c,
								bool allow_int32_type) const
{
	return FindRGBColor(name, 0, c, allow_int32_type);
}

status_t BMessage::FindRGBColor(const char *name, int32 index, rgb_color *c,
								bool allow_int32_type) const
{
	if (index < 0) return B_BAD_INDEX;
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, index);
	if (e != B_OK) return e;
	
	if (info.type != B_RGB_COLOR_TYPE &&
			(!allow_int32_type || info.type != B_INT32_TYPE)) {
		return B_BAD_TYPE;
	}
	
	if (info.size != sizeof(rgb_color)) return B_MISMATCHED_VALUES;
	
	*c = *(const rgb_color*)info.data;
	
	return B_OK;
}

bool BMessage::HasRGBColor(const char *name, int32 n,
							bool allow_int32_type) const
{
	if (n < 0) return false;
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, n);
	if (e != B_OK) return false;
	
	if (info.type != B_RGB_COLOR_TYPE &&
			(!allow_int32_type || info.type != B_INT32_TYPE)) {
		return false;
	}
	
	if (info.size != sizeof(rgb_color)) return false;
	
	return true;
}

status_t BMessage::ReplaceRGBColor(const char *name, rgb_color new_color,
									bool allow_int32_type)
{
	return ReplaceRGBColor(name, 0, new_color, allow_int32_type);
}

status_t BMessage::ReplaceRGBColor(const char *name, int32 index, rgb_color new_color,
									bool allow_int32_type)
{
	if (index < 0) return B_BAD_INDEX;
	
	if (allow_int32_type) {
		// We'll use B_ANY_TYPE to do the replace, so first need to check
		// that the current item is a valid type.
		BMessageBody::data_info info;
		status_t e = fBody->FindData(name, &info, index);
		if (e != B_OK) {
			return e;
		}
		if (info.type != B_RGB_COLOR_TYPE && info.type != B_INT32_TYPE) {
			return B_BAD_TYPE;
		}
	}
	
	return fast_replace_data(name, allow_int32_type ? B_ANY_TYPE : B_RGB_COLOR_TYPE,
					   index, &new_color, sizeof(rgb_color));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddPointer(const char *name, const void *obj)
{
	return fast_add_data(name, B_POINTER_TYPE, &obj, sizeof(void *));
} 

status_t BMessage::FindPointer(const char *name, void **obj) const
{
	return FindPointer(name, 0, obj);
}

status_t BMessage::FindPointer(const char *name, int32 index, void **obj) const
{
	const status_t e = fBody->CopyData(name, B_POINTER_TYPE, index, obj, sizeof(void*));
	if (e != B_OK) *obj = NULL;
	return e;
}

bool BMessage::HasPointer(const char *name, int32 index) const
{
	return(HasData(name, B_POINTER_TYPE, index));
}

status_t BMessage::ReplacePointer(const char *name, const void *obj)
{
	return fast_replace_data(name, B_POINTER_TYPE, 0, &obj, sizeof(void *));
}

status_t BMessage::ReplacePointer(const char *name, int32 index,
	const void *obj)
{
	return fast_replace_data(name, B_POINTER_TYPE, index, &obj, sizeof(void *));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddMessenger(const char *name, BMessenger messenger)
{
	return fast_add_data(name, B_MESSENGER_TYPE, &messenger, sizeof(BMessenger));
} 

status_t BMessage::FindMessenger(const char *name, BMessenger *mess) const
{
	return FindMessenger(name, 0, mess);
}

status_t BMessage::FindMessenger(const char *name, int32 index,
	BMessenger *mess) const
{
	const status_t e = fBody->CopyData(name, B_MESSENGER_TYPE, index, mess, sizeof(BMessenger));
	if (e != B_OK) *mess = BMessenger();
	return e;
}

bool BMessage::HasMessenger(const char *name, int32 index) const
{
	return(HasData(name, B_MESSENGER_TYPE, index));
}

status_t BMessage::ReplaceMessenger(const char *name, BMessenger Messenger)
{
	return fast_replace_data(name, B_MESSENGER_TYPE, 0, &Messenger,
		sizeof(BMessenger));
}

status_t BMessage::ReplaceMessenger(const char *name, int32 index,
	BMessenger Messenger)
{
	return fast_replace_data(name, B_MESSENGER_TYPE, index, &Messenger,
		sizeof(BMessenger));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddRef(const char *name, const entry_ref *ref)
{
	char        buf[MAX_ENTRY_REF_SIZE];
	size_t		size;
	
	status_t err = entry_ref_flatten(buf, &size, ref);
	if (err != B_OK) return err;
	
	return fast_add_data(name, B_REF_TYPE, buf, size, false);
}

status_t BMessage::FindRef(const char *name, entry_ref *ref) const
{
	return FindRef(name, 0, ref);
}

status_t BMessage::FindRef(const char *name, int32 index, entry_ref *ref) const
{
	ssize_t		size;
	const char	*data;
	status_t	err;

	err = fast_find_data(name, B_REF_TYPE, index, (const void **)&data, &size);
	if (!data)
		return err;

	return entry_ref_unflatten(ref, data, size);
}

bool BMessage::HasRef(const char *name, int32 index) const
{
	return(HasData(name, B_REF_TYPE, index));
}

status_t BMessage::ReplaceRef(const char *name, const entry_ref *ref)
{
	return ReplaceRef(name, 0, ref);
}

status_t BMessage::ReplaceRef(const char *name, int32 index, const entry_ref *ref)
{
	char        buf[MAX_ENTRY_REF_SIZE];
	size_t		size;
	
	status_t err = entry_ref_flatten(buf, &size, ref);
	if (err != B_OK) return err;
	
	return fast_replace_data(name, B_REF_TYPE, index, buf, size);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddMessage(const char *name, const BMessage *msg)
{
	START_TIMING();
	
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	if (msg == this) {
		debugger("Can't add a message to itself");
		return B_BAD_VALUE;
	}
	
	const ssize_t size = msg->FlattenedSize();
	if (size < B_OK) return size;
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = B_MESSAGE_TYPE;
	info.size = size;
	
	status_t e;
	fBody = body = body->AllocData(name, &info, false, &e);
	if (e != B_OK) return e;
	
	// This adds references to any atoms in the message while
	// it is being copied.
	e = msg->flatten_no_check((char*)(info.data), size);

	if (e == B_OK && msg->fBody->CountAtoms()) {
		// Now increment our count of atom fields to include the
		// new fields from the source message.
		body->BumpAtoms(msg->fBody->CountAtoms());
	}
	
	NOTE_TIMING(this, Add);
	
	return e;
} 

status_t BMessage::FindMessage(const char *name, BMessage *msg) const
{
	return FindMessage(name, 0, msg);
}

status_t BMessage::FindMessage(const char *name, int32 index,
	BMessage *msg) const
{
	const void	*ptr;
	int32		size;
	status_t	error;

	if ((error = fast_find_data(name, B_MESSAGE_TYPE, index, &ptr, &size)) == B_OK) {
		// This adds references to atoms found in the flattened data.
		error = msg->Unflatten((const char *) ptr);
	}
	return error;
}

bool BMessage::HasMessage(const char *name, int32 index) const
{
	return(HasData(name, B_MESSAGE_TYPE, index));
}

status_t BMessage::ReplaceMessage(const char *name, const BMessage *msg)
{
	return ReplaceMessage(name, 0, msg);
}

status_t BMessage::ReplaceMessage(const char *name, int32 index,
	const BMessage *msg)
{
	START_TIMING();
	
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	const ssize_t size = msg->FlattenedSize();
	if (size < B_OK) return size;
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = B_MESSAGE_TYPE;
	info.size = size;
	
	status_t e;
	fBody = body = body->ReAllocData(name, &info, index, &e);
	if (e != B_OK) return e;
	
	// This adds references to any atoms in the message while
	// it is being copied.
	e = msg->flatten_no_check((char*)(info.data), size);
	
	if (e == B_OK && msg->fBody->CountAtoms()) {
		// Now increment our count of atom fields to include the
		// new fields from the source message.
		body->BumpAtoms(msg->fBody->CountAtoms());
	}
	
	NOTE_TIMING(this, Replace);
	
	return e;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddFlat(const char *name, const BFlattenable *obj, int32 /*num*/)
{
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	const type_code type = obj->TypeCode();
	const ssize_t size = obj->FlattenedSize();
	if (size < B_OK) return size;
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = type;
	info.size = size;
	
	status_t e;
	fBody = body->AllocData(name, &info, obj->IsFixedSize(), &e);
	if (e != B_OK) return e;
	
	return obj->Flatten((char*)(info.data), size);
}

status_t BMessage::FindFlat(const char *name, BFlattenable *obj) const
{
	return FindFlat(name, 0, obj);
}

status_t BMessage::FindFlat(const char *name, int32 index, BFlattenable *obj) const
{
	if (index < 0)
		return B_BAD_INDEX;
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, index);
	if (e != B_OK)
		return e;
	
	if (!obj->AllowsTypeCode(info.type))
		return B_BAD_TYPE;
	
	return obj->Unflatten(info.type, info.data, info.size);
}

bool BMessage::HasFlat(const char *name, const BFlattenable *obj) const
{
	return HasFlat(name, 0, obj);
}

bool BMessage::HasFlat(const char *name, int32, const BFlattenable *obj) const
{
	type_code	c;
	status_t	err;
	int32		count;

	err = GetInfo(name, &c, &count);
	if (err)
		return false;

	if (obj->AllowsTypeCode(c))
		return false;

	return true;
}

status_t BMessage::ReplaceFlat(const char *name, const BFlattenable *obj)
{
	return ReplaceFlat(name, 0, obj);
}

status_t BMessage::ReplaceFlat(const char *name, int32 index, const BFlattenable *obj)
{
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	const type_code type = obj->TypeCode();
	const ssize_t size = obj->FlattenedSize();
	if (size < B_OK) return size;
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = type;
	info.size = size;
	
	status_t e;
	fBody = body->ReAllocData(name, &info, index, &e);
	if (e != B_OK) return e;
	
	return obj->Flatten((char*)(info.data), size);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddAtom(const char *name, const BAtom* atom)
{
	return AddData(name, B_ATOM_TYPE, &atom, sizeof(BAtom *));
} 

status_t BMessage::FindAtom(const char *name, BAtom **atom) const
{
	return FindAtom(name, 0, atom);
}

status_t BMessage::FindAtom(const char *name, int32 index, BAtom **atom) const
{
	const status_t e = fBody->CopyData(name, B_ATOM_TYPE, index, atom, sizeof(BAtom*));
	if (e != B_OK) *atom = NULL;
	return e;
}

bool BMessage::HasAtom(const char *name, int32 n) const
{
	return(HasData(name, B_ATOM_TYPE, n));
}

status_t BMessage::ReplaceAtom(const char *name, const BAtom *atom)
{
	return ReplaceData(name, B_ATOM_TYPE, 0, &atom, sizeof(BAtom *));
}

status_t BMessage::ReplaceAtom(const char *name, int32 index,
	const BAtom *atom)
{
	return ReplaceData(name, B_ATOM_TYPE, index, &atom, sizeof(BAtom *));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::AddAtomRef(const char *name, const BAtom* atom)
{
	return AddData(name, B_ATOMREF_TYPE, &atom, sizeof(BAtom *));
} 

status_t BMessage::FindAtomRef(const char *name, BAtom **atom) const
{
	return FindAtomRef(name, 0, atom);
}

status_t BMessage::FindAtomRef(const char *name, int32 index, BAtom **atom) const
{
	if (index < 0) return B_BAD_INDEX;
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, index);
	if (e != B_OK) return e;
	
	if (info.type != B_ATOMREF_TYPE && info.type != B_ATOM_TYPE) {
		return B_BAD_TYPE;
	}
	
	if (info.size != sizeof(BAtom*)) return B_MISMATCHED_VALUES;
	
	*atom = *(BAtom**)info.data;
	
	return B_OK;
}

bool BMessage::HasAtomRef(const char *name, int32 n) const
{
	if (n < 0) return false;
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, n);
	if (e != B_OK) return false;
	
	if (info.type != B_ATOMREF_TYPE && info.type != B_ATOM_TYPE) {
		return false;
	}
	
	if (info.size != sizeof(BAtom*)) return false;
	
	return true;
}

status_t BMessage::ReplaceAtomRef(const char *name, const BAtom *atom)
{
	return ReplaceData(name, B_ATOMREF_TYPE, 0, &atom, sizeof(BAtom *));
}

status_t BMessage::ReplaceAtomRef(const char *name, int32 index,
	const BAtom *atom)
{
	return ReplaceData(name, B_ATOMREF_TYPE, index, &atom, sizeof(BAtom *));
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::fast_add_data(const char *name, type_code type, const void *data,
	ssize_t numBytes, bool is_fixed_size)
{
	START_TIMING();
	
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = type;
	info.size = numBytes;
	info.data = data;
	
	status_t e;
	fBody = body->FastAddData(name, &info, is_fixed_size, &e);
	
	#if TIMING
	if (e == B_OK) {
		NOTE_TIMING(this, Add);
	}
	#endif
	
	return e;
}

status_t BMessage::AddData(const char *name, type_code type, const void *data,
	ssize_t numBytes, bool is_fixed_size, int32 /*num_adds*/)
{
	START_TIMING();
	
	if (numBytes <= 0) {
		return B_BAD_VALUE;
	}

	if (type == B_INT64_TYPE && strcmp(name, "when") == 0 && numBytes == sizeof(int64)) {
		// Special case -- a single "when" is not stored in the
		// message body.
		const bool in_data = HasInt64("when");
		if (!fHasWhen) {
			if (!in_data) {
				memcpy(&fWhen, data, sizeof(int64));
				fHasWhen = true;
				return B_OK;
			}
		} else if (!in_data) {
			BMessageBody* body = edit_body();
			if (!body)
				return B_NO_MEMORY;
			
			BMessageBody::data_info info;
			info.type = B_INT64_TYPE;
			info.size = sizeof(int64);
			info.data = &fWhen;
			
			status_t e;
			fBody = body->AddData(name, &info, is_fixed_size, &e);
			if (e != B_OK) return e;
		}
	}
	
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = type;
	info.size = numBytes;
	info.data = data;
	
	status_t e;
	fBody = body->AddData(name, &info, is_fixed_size, &e);
	
	#if TIMING
	if (e == B_OK) {
		NOTE_TIMING(this, Add);
	}
	#endif
	
	return e;
}

/* ----------------------------------------------------------------- */

status_t BMessage::fast_find_data(const char *name, type_code type, int32 index,
	const void **data, ssize_t *size) const
{
	if (index < 0) {
		*data = NULL;
		*size = 0;
		return B_BAD_INDEX;
	}
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, index);
	if (e != B_OK) {
		*data = NULL;
		*size = 0;
		return e;
	}
	
	if (!cmp_types(info.type, type)) {
		*data = NULL;
		*size = 0;
		return B_BAD_TYPE;
	}
	
	*data = info.data;
	*size = info.size;
	
	return B_OK;
}

status_t BMessage::FindData(const char *name, type_code type, int32 index,
	const void **data, ssize_t *size) const
{
	if (type == B_INT64_TYPE && index == 0 && fHasWhen && strcmp(name, "when") == 0) {
		*data = &fWhen;
		*size = sizeof(int64);
		return B_OK;
	}
	
	return fast_find_data(name, type, index, data, size);
}

status_t BMessage::FindData(const char *name, type_code type, const void **data,
	ssize_t *size) const
{
	return FindData(name, type, 0, data, size);
}

/* ----------------------------------------------------------------- */

bool BMessage::HasData(const char *name, type_code type, int32 index) const
{
	if (index < 0) return false;
	
	if (type == B_INT64_TYPE && index == 0 && fHasWhen && strcmp(name, "when") == 0) {
		return true;
	}
	
	BMessageBody::data_info info;
	const status_t e = fBody->FindData(name, &info, index);
	if (e != B_OK) return false;
	
	if (!cmp_types(info.type, type)) return false;
	
	return true;
}

/* ----------------------------------------------------------------- */

status_t BMessage::fast_replace_data(const char *name, type_code type, int32 index, 
	const void *data, ssize_t data_size)
{
	START_TIMING();
	
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	if (index < 0) {
		return B_BAD_INDEX;
	}

	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = type;
	info.size = data_size;
	info.data = data;
	
	status_t e;
	fBody = body->FastReplaceData(name, &info, index, &e);
	
	#if TIMING
	if (e == B_OK) {
		NOTE_TIMING(this, Replace);
	}
	#endif
	
	return e;
}

status_t BMessage::ReplaceData(const char *name, type_code type, int32 index, 
	const void *data, ssize_t data_size)
{
	START_TIMING();
	
	if (data_size <= 0) {
		return B_BAD_VALUE;
	}

	if (type == B_INT64_TYPE && index == 0 && data_size == sizeof(int64)
			&& fHasWhen && strcmp(name, "when") == 0) {
		memcpy(&fWhen, data, sizeof(int64));
		return B_OK;
	}
	
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	if (index < 0) {
		return B_BAD_INDEX;
	}

	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	BMessageBody::data_info info;
	info.type = type;
	info.size = data_size;
	info.data = data;
	
	status_t e;
	fBody = body->ReplaceData(name, &info, index, &e);
	
	#if TIMING
	if (e == B_OK) {
		NOTE_TIMING(this, Replace);
	}
	#endif
	
	return e;
}

status_t BMessage::ReplaceData(const char *name, type_code type, const void *data,
	ssize_t data_size)
{
	return ReplaceData(name, type, 0, data, data_size);
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

status_t BMessage::RemoveData(const char *name, int32 index)
{
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	if (index < 0) {
		return B_BAD_INDEX;
	}

	const bool is_when = (strcmp(name, "when") == 0);
	if (is_when) {
		if (index == 0 && fHasWhen) {
			fHasWhen = false;
			fWhen = 0;
			return B_OK;
		}
	}
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	status_t e;
	fBody = body->FreeData(name, index, &e);
	
	if (is_when && e == B_OK) {
		// This is gross, but it shouldn't happen too often.  We
		// need to adjust our fWhen and fHasWhen state based on
		// what happened to an array of "when" items.
		BMessageBody::data_info info;
		const status_t e = fBody->FindData(name, &info, 0);
		if (e == B_OK && info.type == B_INT64_TYPE && info.size == sizeof(int64)) {
			if (info.count == 0) {
				fWhen = 0;
				fHasWhen = false;
			} else if (info.count == 1) {
				bigtime_t when;
				memcpy(&when, info.data, sizeof(int64));
				RemoveName("when");
				fWhen = when;
				fHasWhen = true;
			}
		}
	}
	
	return e;
}

/* ----------------------------------------------------------------- */

status_t BMessage::RemoveName(const char *name)
{
	if (fReadOnly) {
		return B_PERMISSION_DENIED;
	}

	if (fHasWhen && strcmp(name, "when") == 0) {
		fWhen = 0;
		fHasWhen = false;
		return B_OK;
	}
	
	BMessageBody* body = edit_body();
	if (!body)
		return B_NO_MEMORY;
	
	status_t e;
	fBody = body->FreeData(name, -1, &e);
	
	return e;
}

/* ----------------------------------------------------------------- */

status_t BMessage::MakeEmpty()
{
	NOTE_SIZE(this, Empty, CountNames(B_ANY_TYPE));
	START_TIMING();
	
	// We now allow MakeEmpty() on read-only messages -- this is okay,
	// we just clear the message, no need to touch the read-only parts.
	if (fBody) {
		if (!fReadOnly) fBody->Release();
		fBody = NULL;
	}

	if (fOriginal) {
		delete fOriginal;
		fOriginal = NULL;
	}
	
	fWhen = 0;
	fCurSpecifier = -1;
	
	fHasWhen = fReadOnly = false;
	
	NOTE_TIMING(this, Empty);
	
	return B_NO_ERROR;
}

/* ----------------------------------------------------------------- */

void BMessage::make_real_empty()
{
	// Case of a non-politically correct program which did not want
	// to reply.
	reply_if_needed();

	MakeEmpty();
	
	*(message_target*)fTarget = message_target();
	fFlattenWithTarget = false;
}

/* ----------------------------------------------------------------- */
/*---------------------------------------------------------------*/

status_t BMessage::start_writing(message_write_context* context,
								 bool include_target, bool fixed_size) const
{
	header_args args;
	get_args(&args);
	return fBody->StartWriting(context, &args,
							   include_target ? (message_target*)fTarget : NULL,
							   fixed_size);
}

void BMessage::finish_writing(message_write_context* context) const
{
	fBody->FinishWriting(context);
}

/*---------------------------------------------------------------*/
extern int32 send_msg_proc_TLS;
typedef status_t (*send_msg_proc)(port_id, void *, int32, uint32, int64, bool);
#include <TLS.h>

status_t BMessage::send_asynchronous(BDirectMessageTarget* direct,
	bigtime_t when, port_id to_port, int32 target_token,
	const BMessenger& reply_to, uint32 target_flags, bigtime_t timeout) const
{
	message_target target;

	target.target = target_token;
	target.reply_port = reply_to.fPort;
	target.reply_target = reply_to.fHandlerToken;
	target.reply_team = reply_to.fTeam;
	target.flags = (target_flags&~(MTF_PREFERRED_REPLY))
				 | MTF_DELIVERED
				 | (reply_to.fPreferredTarget ? MTF_PREFERRED_REPLY : 0);
	
	send_msg_proc proc = (send_msg_proc)tls_get(send_msg_proc_TLS);
	if (proc) {
		message_write_context context;
		header_args args;
		get_args(&args, when);
		status_t result = fBody->StartWriting(&context, &args, &target);
		if (result == B_OK) {
			result = proc(to_port,(void*)context.data,context.size,
							(target_flags&~MTF_MASK),timeout,
							(target_flags&MTF_SOURCE_IS_WAITING) ? true : false);
		}
		fBody->FinishWriting(&context);
		return result;
	} else if (direct) {
		BMessage* sent = new BMessage(*this);
		if (!sent) return B_NO_MEMORY;
		memcpy(sent->fTarget, &target, sizeof(target));
		sent->SetWhen(when);
		return direct->EnqueueMessage(sent);
	} else {
		header_args args;
		get_args(&args, when);
		return fBody->WritePort(to_port, STD_SEND_MSG_CODE, &args, &target,
								(target_flags&~MTF_MASK), timeout);
	}
}

/*---------------------------------------------------------------*/

status_t BMessage::send_synchronous(BDirectMessageTarget* direct,
	bigtime_t when, port_id port, team_id powner, int32 target_token, uint32 target_flags,
	BMessage *reply, bigtime_t send_timeout, bigtime_t reply_timeout) const
{
	int32				type_code;
	BSyncReplyTarget*	tmp_port = NULL;
	int					cached_port_index = -1;
	team_id				self;
	status_t			err;

	/*
	 We're doing a synchronous send, waiting on a temp port for the reply.
	 There are many race conditions here, with the sender or the receiver
	 dying at some inappropriate time. The code solves these problems as
	 follows: (We are sending from XX to YY)
	 
	 XX creates the tmp_port and gives ownership to YY. XX will
	 call read_port(tmp_port) so if YY dies the tmp_port will be deleted,
	 unblocking XX. Once YY replies it will give ownership back to
	 XX (see SendReply code). When XX finishes reading the data it will
	 delete the port.
	 
	 NOTE: This actually is not safe if the thread receiving this reply
	 dies but its team lives on.  To address this well, we probably need
	 some mechanism in the kernel for threads to own ports.
	*/
	cached_port_index = sGetCachedReplyPort();
	if (cached_port_index != -1) {
		tmp_port = sReplyPorts[cached_port_index];
	} else {
#if SUPPORTS_STREAM_IO
		BErr << "Ran out of tmp_rports" << endl;
#endif
		tmp_port = new(std::nothrow) BSyncReplyTarget("extra_tmprport");
		if (tmp_port == NULL)
			return B_NO_MEMORY;
		if (tmp_port->Port() < 0) {
			status_t result = tmp_port->Port();
			tmp_port->Disconnect();
			return result;
		}
	}

	self = _find_cur_team_id_();
	tmp_port->SetReplyMessage(reply);
	
	// transfer ownership of the 'reply' port to the other guy.
	err = set_port_owner(tmp_port->Port(), powner);
	if (err != B_NO_ERROR)
		goto error_exit;

	{
		// limit the scope of 'rm'
		BMessenger rm(self, tmp_port->Port(), tmp_port->Token(), false);
		err = send_asynchronous(direct, when, port, target_token, rm,
								target_flags | MTF_SOURCE_IS_WAITING, send_timeout);
	}

	if (err) {
		goto error_exit;
	}

	// this won't return until we get the reply. No other message
	// can come over this port.
	for (;;) {
		ssize_t size;
		while ((size = port_buffer_size_etc(tmp_port->Port(),
											(target_flags&~MTF_MASK), reply_timeout))
					== B_INTERRUPTED)
			;
		if (size == 0) {
			if (tmp_port->ReplyReceived())
				break;
		} else {
			err = reply->ReadPort(tmp_port->Port(), size, &type_code,
								  (target_flags&~MTF_MASK), reply_timeout);
			break;
		}
	}
	
	tmp_port->SetReplyMessage(NULL);
	
	if (err) {
		// error occurred. Delete the tmp_port. If the tmp_port
		// was out of the permanent cache then it needs to be replaced.
		if (cached_port_index >= 0) {
#if SUPPORTS_STREAM_IO
			BErr << "Replacing BSyncReplyTarget: " << strerror(err) << endl;
#endif
			sReplyPorts[cached_port_index] = new BSyncReplyTarget("replacement_tmp_rport");
			atomic_add(&(sReplyPortInUse[cached_port_index]), -1);
			cached_port_index = -1;		// free the old one
		}
	}

error_exit:
	if (cached_port_index >= 0) {
		// restore ownership of the cached port to me!
		set_port_owner(tmp_port->Port(), self);
		// release this entry
		atomic_add(&(sReplyPortInUse[cached_port_index]), -1);
	} else {
		tmp_port->Disconnect();
	}

	return(err);
}

/* ----------------------------------------------------------------- */

const BPrivate::message_target&	BMessage::target_struct() const
{
	return *(const message_target*)fTarget;
}

BPrivate::message_target& BMessage::target_struct()
{
	return *(message_target*)fTarget;
}

void BMessage::set_body(const BPrivate::BMessageBody* body)	{ fBody = body; }
const BPrivate::BMessageBody* BMessage::body() const		{ return fBody; }
void BMessage::set_read_only(bool state)					{ fReadOnly = state; }
bool BMessage::read_only() const							{ return fReadOnly; }
void BMessage::set_flatten_with_target(bool state)			{ fFlattenWithTarget = state; }
bool BMessage::flatten_with_target() const					{ return fFlattenWithTarget; }

/* ----------------------------------------------------------------- */

BMessage *_reconstruct_msg_(uint32 what, uint32 serverBase)
{
	BMessage *new_msg = new BMessage(what);

	/* This tells the message not to free its data when it is deleted */
	new_msg->fReadOnly = true;

	/* The base of the area is where our data begins */
	new_msg->fBody = (BMessageBody*)serverBase;

	return new_msg;
}

/* ----------------------------------------------------------------- */

BSyncReplyTarget*	BMessage::sReplyPorts[BMessage::sNumReplyPorts];
int32				BMessage::sReplyPortInUse[BMessage::sNumReplyPorts];

/* ----------------------------------------------------------------- */

extern "C" int _init_message_();
extern "C" int _delete_message_();

int _init_message_()
{
	BMessage::sReplyPorts[0] = new BSyncReplyTarget("tmp_rport0");
	BMessage::sReplyPorts[1] = new BSyncReplyTarget("tmp_rport1");
	BMessage::sReplyPorts[2] = new BSyncReplyTarget("tmp_rport2");

//+	thread_info tinfo;
//+	get_thread_info(find_thread(NULL), &tinfo);
//+
//+	SERIAL_PRINT(("(%d,%d, %s) PORTS: %d, %d, %d\n",
//+		tinfo.team, find_thread(NULL), tinfo.name,
//+		BMessage::sReplyPorts[0],
//+		BMessage::sReplyPorts[1],
//+		BMessage::sReplyPorts[2]));

	BMessage::sReplyPortInUse[0] = 0;
	BMessage::sReplyPortInUse[1] = 0;
	BMessage::sReplyPortInUse[2] = 0;
	return 0;
}

/* ----------------------------------------------------------------- */

int _delete_message_()
{
	BMessage::sReplyPorts[0]->Disconnect();
	BMessage::sReplyPorts[0] = NULL;
	BMessage::sReplyPorts[1]->Disconnect();
	BMessage::sReplyPorts[1] = NULL;
	BMessage::sReplyPorts[2]->Disconnect();
	BMessage::sReplyPorts[2] = NULL;
	return 0;
}

/* ----------------------------------------------------------------- */

int32 BMessage::sGetCachedReplyPort()
{
	int index = -1;
	for (int i = 0; i < sNumReplyPorts; i++) {
		long old = atomic_add(&(sReplyPortInUse[i]), 1);
		if (old == 0) {
			// got it. No one is using this entry.
			index = i;
			break;
		} else {
			// this entry is in use.
			atomic_add(&(sReplyPortInUse[i]), -1);
		}
	}

	return index;
}

/* ----------------------------------------------------------------- */

void BMessage::_ReservedMessage1() {}
void BMessage::_ReservedMessage2() {}
void BMessage::_ReservedMessage3() {}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

#if _R4_5_COMPATIBLE_
extern "C" {

	_EXPORT void
	#if __GNUC__
	__8BMessageP8BMessage
	#elif __MWERKS__
	__ct__8BMessageFP8BMessage
	#endif
	(void* addr, const BMessage* src)
	{
		new(addr) BMessage(*src);
	}

}
#endif

// --------- Deprecated BMessage methods 05/2000 (Dano) ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	AddFlat__8BMessagePCcP12BFlattenablel
	#elif __MWERKS__
	AddFlat__8BMessageFPCcP12BFlattenablel
	#endif
	(BMessage* This, const char* name, BFlattenable* flat, int32 count)
	{
		return This->AddFlat(name, flat, count);
	}
	
	_EXPORT status_t
	#if __GNUC__
	ReplaceFlat__8BMessagePCcP12BFlattenable
	#elif __MWERKS__
	ReplaceFlat__8BMessageFPCcP12BFlattenable
	#endif
	(BMessage* This, const char* name, BFlattenable* flat)
	{
		return This->ReplaceFlat(name, flat);
	}
	
	_EXPORT status_t
	#if __GNUC__
	ReplaceFlat__8BMessagePCclP12BFlattenable
	#elif __MWERKS__
	ReplaceFlat__8BMessageFPCclP12BFlattenable
	#endif
	(BMessage* This, const char* name, int32 index, BFlattenable* flat)
	{
		return This->ReplaceFlat(name, index, flat);
	}
	
	_EXPORT status_t
	#if __GNUC__
	GetInfo__C8BMessageUllPPcPUlPl
	#elif __MWERKS__
	GetInfo__C8BMessageFUllPPcPUlPl
	#endif
	(BMessage* This, type_code tReq, int32 which, char **name, type_code *tRet, int32 *count)
	{
		return This->GetInfo(tReq, which, (const char**)name, tRet, count);
	}
	
}
#endif

// Temporary compatibility with IAD.
extern "C" {
	_EXPORT status_t
	GetNextName__C8BMessagePPvPPcPUlPl
	(BMessage* This, void **cookie, char **outName, type_code *outType, int32 *outCount)
	{
		return This->GetNextName(cookie, (const char**)outName, outType, outCount);
	}
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
