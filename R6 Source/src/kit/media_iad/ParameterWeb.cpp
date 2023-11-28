/*	ParameterWeb.cpp	*/


#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <List.h>
#include <TypeConstants.h>

#include "ParameterWeb.h"
#include "tr_debug.h"
#include "trinity_p.h"
#include "smart_ptr.h"


#define VERSION 1
#define ENDIAN 0x01030506
#define ENDIAN_2 0x02040607
#define ENDIAN_3 0x03040507


#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF
#endif	//	NDEBUG

#if 0
#if defined(DEBUG)
#undef DEBUG
#endif
#define DEBUG 0
#endif

#if DEBUG
static BLocker sLock("ParameterWeb List");
static BList sWebs;
static void _check_web(const BParameterWeb *w, const void *that);
static void _check_class(const char * arg, const char * cls);
#define CHECK_WEB(w) _check_web(w, this)
#if PARAMETER_WEB_NAMES
#define DATA_NAME(x) { _check_class(#x, typeid(*this).name()); strncpy(_name_begin_ ## x , #x , 32); strncpy(_name_end_ ## x , #x , 32); }
#else
#define DATA_NAME(x) { _check_class(#x, typeid(*this).name()); }
#endif
#else
#define CHECK_WEB(w) (void)0
#define DATA_NAME(x)
#endif

#pragma mark --- BParameterWeb ---



#if !MWLD_CLASS_STATIC_DATA_WORKS
#define NO_B_PARAMETER
#else
#define NO_B_PARAMETER BParameter::
#endif

/* These are not necessarily meant to be user-readable; they are identifiers for various */
/* kinds of uses for controls that a Node can make so that an App can understand it. */
const char * const NO_B_PARAMETER B_GENERIC = "";
	/* kinds used for sliders */
const char * const NO_B_PARAMETER B_MASTER_GAIN = "Master";	/* Main Volume */
const char * const NO_B_PARAMETER B_GAIN = "Gain";
const char * const NO_B_PARAMETER B_BALANCE = "Balance";
const char * const NO_B_PARAMETER B_FREQUENCY = "Frequency";	/* like a radio tuner */
const char * const NO_B_PARAMETER B_LEVEL = "Level";		/* like for EQ & effects */
const char * const NO_B_PARAMETER B_SHUTTLE_SPEED = "Speed";	/* Play, SloMo, ... 1.0 == regular */
const char * const NO_B_PARAMETER B_CROSSFADE = "XFade";	/* for raw_video and raw_audio mixers; 0 == first, +100 == second */
const char * const NO_B_PARAMETER B_EQUALIZATION = "EQ";	/* depth (dB) */
	/* compression */
const char * const NO_B_PARAMETER B_COMPRESSION = "Compression";	/* 0% == no compression, 99% == 100:1 compression */
const char * const NO_B_PARAMETER B_QUALITY = "Quality";		/* 0% == full compression, 100% == no compression */
const char * const NO_B_PARAMETER B_BITRATE = "Bitrate";		/* in bits/second */
const char * const NO_B_PARAMETER B_GOP_SIZE = "GOPSize";			/* Group Of Pictures. a k a "Keyframe every N frames" */
	/* kinds used for selectors */
const char * const NO_B_PARAMETER B_MUTE = "Mute";		/* 0 == thru, 1 == mute */
const char * const NO_B_PARAMETER B_ENABLE = "Enable";		/* 0 == disable, 1 == enable */
const char * const NO_B_PARAMETER B_INPUT_MUX = "Input";
const char * const NO_B_PARAMETER B_OUTPUT_MUX = "Output";
const char * const NO_B_PARAMETER B_TUNER_CHANNEL = "Channel";		/* like cable TV */
const char * const NO_B_PARAMETER B_TRACK = "Track";		/* like a CD player */
const char * const NO_B_PARAMETER B_RECSTATE = "RecState";	/* like mutitrack tape deck, 0 == silent, 1 == play, 2 == record */
const char * const NO_B_PARAMETER B_SHUTTLE_MODE = "Shuttle";	/* -1 == backwards, 0 == stop, 1 == play, 2 == pause/cue */
const char * const NO_B_PARAMETER B_RESOLUTION = "Resolution";
const char * const NO_B_PARAMETER B_COLOR_SPACE = "Colorspace";
const char * const NO_B_PARAMETER B_FRAME_RATE = "FrameRate";
const char * const NO_B_PARAMETER B_VIDEO_FORMAT = "VideoFormat";	/* 1 == NTSC-M, 2 == NTSC-J, 3 == PAL-BDGHI, 4 == PAL-M, 5 == PAL-N, 6 == SECAM, 7 == MPEG-1, 8 == MPEG-2 */
	/* junctions */
const char * const NO_B_PARAMETER B_WEB_PHYSICAL_INPUT = "PhysInput";
const char * const NO_B_PARAMETER B_WEB_PHYSICAL_OUTPUT = "PhysOutput";
const char * const NO_B_PARAMETER B_WEB_ADC_CONVERTER = "ADC";
const char * const NO_B_PARAMETER B_WEB_DAC_CONVERTER = "DAC";
const char * const NO_B_PARAMETER B_WEB_LOGICAL_INPUT = "LogInput";
const char * const NO_B_PARAMETER B_WEB_LOGICAL_OUTPUT = "LogOutput";
const char * const NO_B_PARAMETER B_WEB_BUFFER_INPUT = "DataInput";
const char * const NO_B_PARAMETER B_WEB_BUFFER_OUTPUT = "DataOutput";

	/* simple transport control */
const char * const NO_B_PARAMETER B_SIMPLE_TRANSPORT = "SimpleTransport";

BParameterWeb::BParameterWeb()
{
	mGroups = new BList;
	mNode = media_node::null;
	mOldRefs = NULL;
	mNewRefs = NULL;
#if DEBUG
	sLock.Lock();
	sWebs.AddItem(this);
	sLock.Unlock();
#endif
	DATA_NAME(BParameterWeb)
#if DEBUG
	free(malloc(1));
#endif
}


BParameterWeb::~BParameterWeb()
{
	CHECK_WEB(this);
	for (int ix=0; ix<mGroups->CountItems(); ix++)
	{
		delete (BParameterGroup *)mGroups->ItemAt(ix);
	}
	delete mGroups;
	delete mOldRefs;
	delete mNewRefs;
#if DEBUG
	sLock.Lock();
	sWebs.RemoveItem(this);
	sLock.Unlock();
#endif
}


media_node
BParameterWeb::Node()
{
	CHECK_WEB(this);
	return mNode;
}


int32
BParameterWeb::CountGroups()
{
	CHECK_WEB(this);
	return mGroups->CountItems();
}


BParameterGroup *
BParameterWeb::GroupAt(
	int32 index)
{
	CHECK_WEB(this);
	return (BParameterGroup *)mGroups->ItemAt(index);
}


BParameterGroup *
BParameterWeb::MakeGroup(
	const char * name)
{
	CHECK_WEB(this);
	ASSERT(name != NULL);
	BParameterGroup * grp = new BParameterGroup(this, name);
	mGroups->AddItem(grp);
	return grp;
}


int32
BParameterWeb::CountParameters()
{
	CHECK_WEB(this);
	/* do a breadth-first count of parameters */
	int index = 0;
	BList groups(*mGroups);
	for (int ix = 0; ix<groups.CountItems(); ix++) {
		BParameterGroup * group = (BParameterGroup *)groups.ItemAt(ix);
		index += group->CountParameters();
		if (group->mGroups) {
			groups.AddList(group->mGroups);
		}
	}
	return index;
}


BParameter *
BParameterWeb::ParameterAt(
	int32 index)
{
	CHECK_WEB(this);
	/* do a breadth-first search for parameters */
	BList groups(*mGroups);
	for (int ix = 0; ix<groups.CountItems(); ix++) {
		BParameterGroup * group = (BParameterGroup *)groups.ItemAt(ix);
		if (index < group->CountParameters()) {
			return group->ParameterAt(index);
		}
		index -= group->CountParameters();
		if (group->mGroups) {
			groups.AddList(group->mGroups);
		}
	}
	return NULL;
}


bool
BParameterWeb::IsFixedSize() const
{
	CHECK_WEB(this);
 	return false;
}


type_code
BParameterWeb::TypeCode() const
{
	CHECK_WEB(this);
	return B_MEDIA_PARAMETER_WEB_TYPE;
}


ssize_t
BParameterWeb::FlattenedSize() const
{
	CHECK_WEB(this);
//	dlog("BParameterWeb::FlattenedSize()");
	size_t total = 3*4+sizeof(mNode);
	for (int ix=0; ix<mGroups->CountItems(); ix++)
	{
		total += 2*4;
		total += ((BParameterGroup *)mGroups->ItemAt(ix))->FlattenedSize();
	}
	return total;
}


#define put_4(b,v) { memcpy(b,&v,4); b=((char*)b)+4; }
#define put_1(b,v) { *(char*)b = v; b=((char*)b)+1; }
#define put_N(b,v,s) { if (s) memcpy(b,v,s); b=((char*)b)+s; }

status_t
BParameterWeb::Flatten(
	void *buffer, 
	ssize_t size) const
{
	CHECK_WEB(this);
	dlog("BParameterWeb::Flatten()");
	if (size < FlattenedSize())
		return B_NO_MEMORY;
	uint32 temp = ENDIAN;
	put_4(buffer, temp);
	temp = VERSION;
	put_4(buffer, temp);
	int32 count = mGroups->CountItems();
	put_4(buffer, count);
	put_N(buffer, &mNode, sizeof(mNode));
	for (int ix=0; ix<count; ix++)
	{
		BParameterGroup * item = ((BParameterGroup *)mGroups->ItemAt(ix));
//		put_4(buffer, item);	/* actual pointer value used for fix-up later */
		temp = item->FlattenedSize();
		put_4(buffer, temp);
		item->Flatten(buffer, size);
		buffer = ((char *)buffer)+temp;
	}
	return B_OK;
}


bool
BParameterWeb::AllowsTypeCode(type_code code) const
{
	CHECK_WEB(this);
	return (code == B_MEDIA_PARAMETER_WEB_TYPE);
}


#define get_4(b,s,v) { if(s<4) return B_NO_MEMORY; memcpy(&v, b, 4); b=((char*)b)+4; s-=4; }
#define get_1(b,s,v) { if(s<1) return B_NO_MEMORY; v = *(char*)b; b=((char*)b)+1; s-=1; }
#define get_str(b,s,p) { if (_get_str(b,s,p)) return B_NO_MEMORY; }
#define get_N(b,s,v,l) { if (s<l) return B_NO_MEMORY; memcpy(v,b,l); b=((char*)b)+l; s-=l; }

	static status_t
	_get_str(
		const void *& b,
		ssize_t & s,
		char ** o)
	{
		unsigned char ch;
		get_1(b, s, ch);
		*o = NULL;
		if (ch) {
			*o = (char*)malloc(ch+1);
			if (!*o) return B_NO_MEMORY;
			memcpy(*o, b, ch);
			(*o)[ch] = 0;
			b = ((char *)b)+ch;
			s -= ch;
		}
//		dlog("string = %s", *o ? *o : "NULL");
		return B_OK;
	}

status_t
BParameterWeb::Unflatten(
	type_code c, 
	const void *buf, 
	ssize_t size)
{
	CHECK_WEB(this);
	dlog("BParameterWeb::Unflatten()");
	bool swap = false;
	if (c != B_MEDIA_PARAMETER_WEB_TYPE) return B_BAD_VALUE;
	uint32 temp;
	get_4(buf, size, temp);
	if (temp != ENDIAN) {
		if (ENDIAN != B_SWAP_INT32(temp))
			return B_BAD_VALUE;
		else
			swap = true;
	}
	get_4(buf, size, temp);
	if (swap) temp = B_SWAP_INT32(temp);
	if (temp != VERSION) return B_BAD_VALUE;
	int32 count;
	get_4(buf, size, count);
	if (swap) count = B_SWAP_INT32(count);
	get_N(buf, size, &mNode, sizeof(mNode));
	if (swap) swap_data(B_INT32_TYPE, &mNode, sizeof(mNode), B_SWAP_ALWAYS);
	if ((count < 0) || (count > 200)) return B_BAD_VALUE;
	for (int ix=0; ix<count; ix++)
	{
		int32 bytes;
		if (size < 4) return B_BAD_VALUE;
		get_4(buf, size, bytes);
		if (swap) bytes = B_SWAP_INT32(bytes);
		if (bytes > size) {
			return B_BAD_VALUE;
		}
#if DEBUG
		fprintf(stderr, "BParameterWeb::Unflatten()::MakeGroup()\n");
#endif
		BParameterGroup * c = MakeGroup("");
		if (!c) return B_BAD_VALUE;
		status_t err = c->Unflatten(B_MEDIA_PARAMETER_GROUP_TYPE, buf, bytes);
		buf = ((char *)buf)+bytes;
		if (err < B_OK) return err;
	}
	/* traverse all groups in breadth fist order, patching up parameter references therein */
	BList groups(*mGroups);
	if (mOldRefs && mNewRefs) for (int ix=0; ix<groups.CountItems(); ix++) {
		BParameterGroup * g = (BParameterGroup *)groups.ItemAt(ix);
		if (g->mGroups) {
			groups.AddList(g->mGroups);
		}
		if (g->mControls) for (int iy=0; iy<g->mControls->CountItems(); iy++) {
			BParameter * p = (BParameter *)g->mControls->ItemAt(iy);
#if DEBUG
		fprintf(stderr, "BParameterWeb::Unflatten()::FixRefs()\n");
#endif
			p->FixRefs(*mOldRefs, *mNewRefs);
		}
	}
	delete mOldRefs;
	delete mNewRefs;
	mOldRefs = NULL;
	mNewRefs = NULL;
	return B_OK;
}


status_t
BParameterWeb::_Reserved_ControlWeb_0(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_1(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_2(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_3(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_4(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_5(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_6(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

status_t
BParameterWeb::_Reserved_ControlWeb_7(void *)
{
	CHECK_WEB(this);
	return B_ERROR;
}

void
BParameterWeb::AddRefFix(
	void * oldItem,
	void * newItem)
{
	CHECK_WEB(this);
	ASSERT((oldItem != NULL) && (newItem != NULL));
	if (!mOldRefs) {
		mOldRefs = new BList;
		ASSERT(!mNewRefs);
	}
	if (!mNewRefs) {
		mNewRefs = new BList;
	}
	mOldRefs->AddItem(oldItem);
	mNewRefs->AddItem(newItem);
}




#pragma mark --- BParameterGroup ---



BParameterGroup::BParameterGroup(
	BParameterWeb * web,
	const char * name)
{
	mWeb = web;
	CHECK_WEB(mWeb);
	mName = ((name && *name) ? strdup(name) : NULL);
	mControls = new BList;
	mGroups = new BList;
	DATA_NAME(BParameterGroup)
}


BParameterGroup::~BParameterGroup()
{
	CHECK_WEB(mWeb);
	for (int ix=0; ix<mControls->CountItems(); ix++)
	{
		BParameter * ctrl = (BParameter *)mControls->ItemAtFast(ix);
		delete ctrl;
	}
	delete mControls;
	for (int ix=0; ix<mGroups->CountItems(); ix++)
	{
		BParameterGroup * grp = (BParameterGroup *)mGroups->ItemAtFast(ix);
		delete grp;
	}
	delete mGroups;
	free(mName);
}


const char *
BParameterGroup::Name() const
{
	CHECK_WEB(mWeb);
	return mName ? mName : "";
}


BParameterWeb * 
BParameterGroup::Web() const
{
	CHECK_WEB(mWeb);
	return mWeb;
}


BNullParameter *
BParameterGroup::MakeNullParameter(
	long id,
	media_type m_type,
	const char * name,
	const char * kind)
{
	CHECK_WEB(mWeb);
	BNullParameter * ret = new BNullParameter(id, m_type, mWeb, name, kind);
	ret->mGroup = this;
	mControls->AddItem((BParameter *)ret);
	return ret;
}


BContinuousParameter *
BParameterGroup::MakeContinuousParameter(
	int32 id,
	media_type m_type,
	const char * name,
	const char * kind,
	const char * unit,
	float minimum,
	float maximum,
	float stepping)
{
	CHECK_WEB(mWeb);
	BContinuousParameter * ret = new BContinuousParameter(id, m_type, mWeb, name, kind, unit, minimum, maximum, stepping);
	ret->mGroup = this;
	mControls->AddItem((BParameter *)ret);
	return ret;
}


BDiscreteParameter *
BParameterGroup::MakeDiscreteParameter(
	int32 id,
	media_type m_type,
	const char * name,
	const char * kind)
{
	CHECK_WEB(mWeb);
	BDiscreteParameter * ret = new BDiscreteParameter(id, m_type, mWeb, name, kind);
	ret->mGroup = this;
	mControls->AddItem((BParameter *)ret);
	return ret;
}


BParameter *
BParameterGroup::MakeControl(
	int32 type)
{
	CHECK_WEB(mWeb);
	BParameter * ret = NULL;
	switch (type)
	{
	case BParameter::B_NULL_PARAMETER:
		ret = MakeNullParameter(0, B_MEDIA_NO_TYPE, NULL, NULL);
		break;
	case BParameter::B_CONTINUOUS_PARAMETER:
		ret = MakeContinuousParameter(0, B_MEDIA_NO_TYPE, NULL, NULL, NULL, 0, 0, 0);
		break;
	case BParameter::B_DISCRETE_PARAMETER:
		ret = MakeDiscreteParameter(0, B_MEDIA_NO_TYPE, NULL, NULL);
		break;
	default:
		break;
	}
	return ret;
}


BParameter *
BParameterGroup::ParameterAt(
	int32 index)
{
	CHECK_WEB(mWeb);
	return (BParameter *)mControls->ItemAt(index);
}


int32
BParameterGroup::CountParameters()
{
	CHECK_WEB(mWeb);
	return mControls->CountItems();
}


BParameterGroup *
BParameterGroup::MakeGroup(
	const char * name)
{
	CHECK_WEB(mWeb);
	BParameterGroup * grp = new BParameterGroup(mWeb, name);
	mGroups->AddItem(grp);
	return grp;
}

int32
BParameterGroup::CountGroups()
{
	CHECK_WEB(mWeb);
	return mGroups->CountItems();
}

BParameterGroup *
BParameterGroup::GroupAt(
	int32 index)
{
	CHECK_WEB(mWeb);
	return (BParameterGroup *)mGroups->ItemAt(index);
}


bool
BParameterGroup::IsFixedSize() const
{
	CHECK_WEB(mWeb);
	return false;
}


type_code
BParameterGroup::TypeCode() const
{
	CHECK_WEB(mWeb);
	return B_MEDIA_PARAMETER_GROUP_TYPE;
}


bool
BParameterGroup::AllowsTypeCode(
	type_code c) const
{
	CHECK_WEB(mWeb);
	return c == B_MEDIA_PARAMETER_GROUP_TYPE;
}


ssize_t
BParameterGroup::FlattenedSize() const
{
	CHECK_WEB(mWeb);
//	dlog("BParameterGroup::FlattenedSize()");
	size_t total = 3*4+(mName ? strlen(mName) : 0)+1;
	for (int ix=0; ix<mControls->CountItems(); ix++)
	{
		total += 3*4;
		total += ((BParameter *)mControls->ItemAt(ix))->FlattenedSize();
	}
	for (int ix=0; ix<mGroups->CountItems(); ix++)
	{
		total += 3*4;
		total += ((BParameterGroup *)mGroups->ItemAt(ix))->FlattenedSize();
	}
	return total;
}


status_t
BParameterGroup::Flatten(
	void *buffer, 
	ssize_t size) const
{
	CHECK_WEB(mWeb);
	//dlog("BParameterGroup::Flatten()");
	if (size < FlattenedSize())
		return B_NO_MEMORY;
	uint32 temp = ENDIAN_3;
	put_4(buffer, temp);
	uchar l = mName ? strlen(mName) : 0;
	put_1(buffer, l);
	if (l) put_N(buffer, mName, l);
	//dlog("name = %x (%s)", mName, mName ? mName : "0");
	int32 count = mControls->CountItems();
	put_4(buffer, count);
	for (int ix=0; ix<count; ix++) {
		BParameter * item = ((BParameter *)mControls->ItemAt(ix));
		put_4(buffer, item);	/* actual pointer value used for fix-up later */
		temp = item->Type();
		put_4(buffer, temp);
		temp = item->FlattenedSize();
		put_4(buffer, temp);
		item->Flatten(buffer, size);
		buffer = ((char *)buffer)+temp;
	}
	count = mGroups->CountItems();
	put_4(buffer, count);
	for (int ix=0; ix<count; ix++) {
		BParameterGroup * grp = ((BParameterGroup *)mGroups->ItemAt(ix));
		put_4(buffer, grp);
		temp = grp->TypeCode();
		put_4(buffer, temp);
		temp = grp->FlattenedSize();
		put_4(buffer, temp);
		grp->Flatten(buffer, size);
		buffer = ((char *)buffer)+temp;
	}
	return B_OK;
}


status_t
BParameterGroup::Unflatten(
	type_code c, 
	const void *buf, 
	ssize_t size)
{
	CHECK_WEB(mWeb);
	//dlog("BParmeterGroup::Unflatten()\n");
	/* read internal data */
	bool swap = false;
	if (c != B_MEDIA_PARAMETER_GROUP_TYPE) return B_BAD_VALUE;
	uint32 temp;
	get_4(buf, size, temp);
	if (temp != ENDIAN_3) {
		if (ENDIAN_3 != B_SWAP_INT32(temp))
			return B_BAD_VALUE;
		else
			swap = true;
	}
	uchar nlen;
	get_1(buf, size, nlen);
	free(mName);
	if (nlen > 0) {
		mName = (char *)malloc(nlen+1);
		get_N(buf, size, mName, nlen);
		mName[nlen] = 0;
	}
	else {
		mName = NULL;
	}
	int32 count;
	/* read parameters */
	get_4(buf, size, count);
	if (swap) count = B_SWAP_INT32(count);
	if ((count < 0) || (count > 10000)) return B_BAD_VALUE;
	for (int ix=0; ix<count; ix++) {
		int32 type, bytes;
		if (size < 12) return B_BAD_VALUE;
		void * item = NULL;
		get_4(buf, size, item);
		if (swap) item = (void*)B_SWAP_INT32((int32)item);
		get_4(buf, size, type);
		if (swap) type = B_SWAP_INT32(type);
		get_4(buf, size, bytes);
		if (swap) bytes = B_SWAP_INT32(bytes);
		if (bytes > size) return B_BAD_VALUE;
#if DEBUG
		fprintf(stderr, "BParameterGroup::Unflatten()::MakeControl()\n");
#endif
		BParameter * c = MakeControl(type);
		if (!c) return B_BAD_VALUE;
		status_t err = c->Unflatten(B_MEDIA_PARAMETER_TYPE, buf, bytes);
		buf = ((char *)buf)+bytes;
		if (err < B_OK) return err;
		mWeb->AddRefFix(item, c);
	}
	/* read sub-groups */
	get_4(buf, size, count);
	if (swap) count = B_SWAP_INT32(count);
	if ((count < 0) || (count > 1000)) return B_BAD_VALUE;
	for (int ix=0; ix<count; ix++) {
		int32 type = 0, bytes = 0;
		void * item = NULL;
		if (size < 12) return B_BAD_VALUE;
		get_4(buf, size, item);
		if (swap) item = (void *)B_SWAP_INT32((int32)item);
		get_4(buf, size, type);
		if (swap) type = B_SWAP_INT32(type);
		get_4(buf, size, bytes);
		if (swap) bytes = B_SWAP_INT32(bytes);
		if (bytes > size) return B_BAD_VALUE;
#if DEBUG
		fprintf(stderr, "BParameterGroup::Unflatten()::MakeGroup()\n");
#endif
		BParameterGroup * c = MakeGroup("");
		if (!c) return B_BAD_VALUE;
		status_t err = c->Unflatten(type, buf, bytes);
		buf = ((char *)buf)+bytes;
		if (err < B_OK) return err;
		Web()->AddRefFix(item, c);
	}
	/* fix up stored pointer references to groups and parameters in BParameterWeb::Unflatten() */
	return B_OK;
}


status_t
BParameterGroup::_Reserved_ControlGroup_0(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_1(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_2(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_3(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_4(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_5(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_6(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameterGroup::_Reserved_ControlGroup_7(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}





#pragma mark --- BParameter ---


BParameter::media_parameter_type
BParameter::Type() const
{
	CHECK_WEB(mWeb);
	return mType;
}


BParameterWeb *
BParameter::Web() const
{
	CHECK_WEB(mWeb);
	return mWeb;
}


BParameterGroup *
BParameter::Group() const
{
	CHECK_WEB(mWeb);
	return mGroup;
}


const char *
BParameter::Name() const
{
	CHECK_WEB(mWeb);
	return mName ? mName : "";
}


const char *
BParameter::Kind() const
{
	CHECK_WEB(mWeb);
	return mKind ? mKind : "";
}


const char *
BParameter::Unit() const
{
	CHECK_WEB(mWeb);
	return mUnit ? mUnit : "";
}


int32
BParameter::ID() const
{
	CHECK_WEB(mWeb);
	return mID;
}


status_t
BParameter::GetValue(
	void * data,
	size_t * ioSize,
	bigtime_t * when)
{
	CHECK_WEB(mWeb);
	dassert((((ValueType() == B_INT32_TYPE) || (ValueType() == B_FLOAT_TYPE)) && (*ioSize >= (CountChannels() * 4))) ||
		((ValueType() == B_BOOL_TYPE) && (*ioSize >= (CountChannels() * 1))));
	smart_ptr<get_values_q> cmd;
	smart_ptr<get_values_a> ans;
	cmd->reply = create_port(1, "BParameter::GetValue");
	cmd->count_ids = 1;
	cmd->ids[0] = mID;
	status_t err = write_port(mWeb->Node().port, CT_GET_VALUES, cmd, 
		sizeof(get_values_q)-sizeof(cmd->ids)+sizeof(int32)*1);
	if (err >= 0) {
		int32 code;
		err = read_port_etc(cmd->reply, &code, ans, sizeof(get_values_a), B_TIMEOUT, DEFAULT_TIMEOUT);
	}
	delete_port(cmd->reply);
	if (err >= 0) err = ans->error;
	if (err >= 0) if (ans->num_values < 1) err = B_BAD_VALUE;
	if (err >= 0)
	{
		struct {
			bigtime_t when;
			int32 id;
			size_t size;
		} hdr;
		memcpy(&hdr, ans->raw_data, sizeof(hdr));
		if (hdr.size > *ioSize)
		{
			err = B_NO_MEMORY;
			hdr.size = *ioSize;
		}
		memcpy(data, &ans->raw_data[sizeof(hdr)], hdr.size);
		*ioSize = hdr.size;
		if (when) *when = hdr.when;	/* work around William's bug */
	}
	return err > 0 ? B_OK : err;
}


status_t
BParameter::SetValue(
	const void * data,
	size_t size,
	bigtime_t when)
{
	CHECK_WEB(mWeb);
	smart_ptr<set_values_q> cmd;
	cmd->node = mWeb->Node();
	cmd->num_values = 1;
	struct {
		bigtime_t when;
		int32 id;
		size_t size;
	} hdr = { when, mID, size };
	memcpy(&cmd->raw_data[sizeof(hdr)], data, size);
	memcpy(cmd->raw_data, &hdr, sizeof(hdr));
	status_t err = write_port(cmd->node.port, CT_SET_VALUES, 
		cmd, sizeof(set_values_q)-sizeof(cmd->raw_data)+sizeof(hdr)+size);
	if (err > 0) err = B_OK;
	return err;
}


int32
BParameter::CountChannels()
{
	CHECK_WEB(mWeb);
	return mChannels;
}


void
BParameter::SetChannelCount(
	int32 channel_count)
{
	CHECK_WEB(mWeb);
	mChannels = channel_count;
}


int32
BParameter::CountInputs()
{
	CHECK_WEB(mWeb);
	return mInputs ? mInputs->CountItems() : 0;
}


BParameter *
BParameter::InputAt(
	int32 index)
{
	CHECK_WEB(mWeb);
	if (!mInputs) return NULL;
	return (BParameter *)mInputs->ItemAt(index);
}


void
BParameter::AddInput(
	BParameter * input)
{
	CHECK_WEB(mWeb);
	if (!mInputs) mInputs = new BList;
	if (!mInputs->HasItem(input)) {
		mInputs->AddItem(input);
		input->AddOutput(this);
	}
}


int32
BParameter::CountOutputs()
{
	CHECK_WEB(mWeb);
	if (!mOutputs) return 0;
	return mOutputs->CountItems();
}


BParameter *
BParameter::OutputAt(
	int32 index)
{
	CHECK_WEB(mWeb);
	if (!mOutputs) return NULL;
	return (BParameter *)mOutputs->ItemAt(index);
}


void
BParameter::AddOutput(
	BParameter * output)
{
	CHECK_WEB(mWeb);
	if (!mOutputs) mOutputs = new BList;
	if (!mOutputs->HasItem(output)) {
		mOutputs->AddItem(output);
		output->AddInput(this);
	}
}


bool
BParameter::IsFixedSize() const
{
	CHECK_WEB(mWeb);
	return false;
}


type_code
BParameter::TypeCode() const
{
	CHECK_WEB(mWeb);
	return B_MEDIA_PARAMETER_TYPE;
}


ssize_t
BParameter::FlattenedSize() const
{
	CHECK_WEB(mWeb);
	//dlog("BParameter::FlattenedSize()");
	size_t total = 3*4;
	total++; if (mName) total += strlen(mName);
	total++; if (mKind) total += strlen(mKind);
	total++; if (mUnit) total += strlen(mUnit);
	total += 4;
	if (mInputs) total += 4*mInputs->CountItems();
	total += 4;
	if (mOutputs) total += 4*mOutputs->CountItems();
	total += 4;	/* media type */
	total += 4;	/* num channel_count */
	return total;
}


status_t
BParameter::Flatten(void *buffer, ssize_t size) const
{
	CHECK_WEB(mWeb);
	//dlog("BParameter::Flatten()");
	if (size < FlattenedSize()) return B_NO_MEMORY;
	int32 temp = ENDIAN_2;
	put_4(buffer, temp);
	size_t bytes = BParameter::FlattenedSize();
	put_4(buffer, bytes);
	put_4(buffer, mID);
	unsigned char l = mName ? strlen(mName) : 0;
	put_1(buffer, l);
	//dlog("name = %x (%s)", mName, mName ? mName : "0");
	put_N(buffer, mName, l);
	l = mKind ? strlen(mKind) : 0;
	put_1(buffer, l);
	put_N(buffer, mKind, l);
	l = mUnit ? strlen(mUnit) : 0;
	put_1(buffer, l);
	put_N(buffer, mUnit, l);
	temp = mInputs ? mInputs->CountItems() : 0;
	put_4(buffer, temp);
	if (mInputs) for (int ix=0; ix<mInputs->CountItems(); ix++)
	{
		void * item = mInputs->ItemAt(ix);
		put_4(buffer, item);
	}
	temp = mOutputs ? mOutputs->CountItems() : 0;
	put_4(buffer, temp);
	if (mOutputs) for (int ix=0; ix<mOutputs->CountItems(); ix++)
	{
		void * item = mOutputs->ItemAt(ix);
		put_4(buffer, item);
	}
	put_4(buffer, mMediaType);
	put_4(buffer, mChannels);
	return B_OK;
}


bool
BParameter::AllowsTypeCode(type_code code) const
{
	CHECK_WEB(mWeb);
	return (code == B_MEDIA_PARAMETER_TYPE);
}


status_t
BParameter::Unflatten(
	type_code c, 
	const void *buffer, 
	ssize_t size)
{
	CHECK_WEB(mWeb);
	//dlog("BParameter::Unflatten()");
	int32 temp = 0;
	get_4(buffer, size, temp);
	if (temp != ENDIAN_2)
		if (B_SWAP_INT32(temp) != ENDIAN_2)
			return B_BAD_VALUE;
		else
			mSwapDetected = true;
	get_4(buffer, size, temp);
	if (mSwapDetected) temp = B_SWAP_INT32(temp);
	size = temp-2*4;	/* how much we really have */
	get_4(buffer, size, mID);
	if (mSwapDetected) mID = B_SWAP_INT32(mID);
	get_str(buffer, size, &mName);
	get_str(buffer, size, &mKind);
	get_str(buffer, size, &mUnit);
	int32 count = 0;
	get_4(buffer, size, count);
	if (mSwapDetected) count = B_SWAP_INT32(count);
	if (count > 0) mInputs = new BList;
	for (int ix=0; ix<count; ix++)
	{
		void * item;
		get_4(buffer, size, item);
		if (mSwapDetected) item = (void*)B_SWAP_INT32((int32)item);
		/* this is not as dangerous as it looks -- check FixRefs() */
		mInputs->AddItem(item);
	}
	get_4(buffer, size, count);
	if (mSwapDetected) count = B_SWAP_INT32(count);
	if (count > 0) mOutputs = new BList;
	for (int ix=0; ix<count; ix++)
	{
		void * item;
		get_4(buffer, size, item);
		if (mSwapDetected) item = (void*)B_SWAP_INT32((int32)item);
		/* this is not as dangerous as it looks -- check FixRefs() */
		mOutputs->AddItem(item);
	}
	get_4(buffer, size, mMediaType);
	if (mSwapDetected) mMediaType = (media_type)B_SWAP_INT32(mMediaType);
	get_4(buffer, size, mChannels);
	if (mSwapDetected) mChannels = B_SWAP_INT32(mChannels);
	return B_OK;
}


media_type
BParameter::MediaType()
{
	CHECK_WEB(mWeb);
	return mMediaType;
}


void
BParameter::SetMediaType(
	media_type m_type)
{
	CHECK_WEB(mWeb);
	mMediaType = m_type;
}



BParameter::BParameter(
	int32 id,
	media_type m_type,
	media_parameter_type type,
	BParameterWeb * web,
	const char * name,
	const char * kind,
	const char * unit)
{
	mWeb = web;
	CHECK_WEB(mWeb);
	mID = id;
	mType = type;
	mGroup = NULL;
	if (name) mName = strdup(name); else mName = NULL;
	if (kind) mKind = strdup(kind); else mKind = NULL;
	if (unit) mUnit = strdup(unit); else mUnit = NULL;
	mInputs = NULL;
	mOutputs = NULL;
	mSwapDetected = false;
	mMediaType = m_type;
	mChannels = 1;
	DATA_NAME(BParameter)
}


BParameter::~BParameter()
{
	CHECK_WEB(mWeb);
	delete mInputs;
	delete mOutputs;
	free(mName);
	free(mKind);
	free(mUnit);
}


void
BParameter::FixRefs(
	BList & old,
	BList & updated)
{
	CHECK_WEB(mWeb);
	/* Any pointer values that were un-flattened should be updated */
	/* according to the table supplied by the owner. */
	if (mInputs) for (int ix=0; ix<mInputs->CountItems(); ix++) {
		void * item = mInputs->ItemAtFast(ix);
		int32 index = old.IndexOf(item);
		if (index < 0) {
			dlog("Removing item %d (%x) from mInputs", ix, item);
			mInputs->RemoveItem(ix);
			ix--;
		}
		else {
			mInputs->ReplaceItem(ix, updated.ItemAt(index));
		}
	}
	if (mOutputs) for (int ix=0; ix<mOutputs->CountItems(); ix++) {
		void * item = mOutputs->ItemAtFast(ix);
		int32 index = old.IndexOf(item);
		if (index < 0) {
			dlog("Removing item %d (%x) from mOutputs", ix, item);
			mOutputs->RemoveItem(ix);
			ix--;
		}
		else {
			mOutputs->ReplaceItem(ix, updated.ItemAt(index));
		}
	}
}


		/* Mmmh, stuffing! */
status_t
BParameter::_Reserved_Control_0(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_1(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_2(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_3(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_4(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_5(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_6(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BParameter::_Reserved_Control_7(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}






#pragma mark --- ContinuousParameter ---





type_code
BContinuousParameter::ValueType()
{
	CHECK_WEB(mWeb);
	return B_FLOAT_TYPE;
}


float
BContinuousParameter::MinValue()
{
	CHECK_WEB(mWeb);
	return mMinimum;
}


float
BContinuousParameter::MaxValue()
{
	CHECK_WEB(mWeb);
	return mMaximum;
}


float
BContinuousParameter::ValueStep()
{
	CHECK_WEB(mWeb);
	return mStepping;
}


void
BContinuousParameter::SetResponse(
	int resp,
	float factor,
	float offset)
{
	mResponse = (response)resp;
	mFactor = factor;
	mOffset = offset;
}


void
BContinuousParameter::GetResponse(
	int * resp,
	float * factor,
	float * offset)
{
	assert(resp != NULL && factor != NULL && offset != NULL);
	if (resp) *resp = mResponse;
	if (factor) *factor = mFactor;
	if (offset) *offset = mOffset;
}


ssize_t
BContinuousParameter::FlattenedSize() const
{
	CHECK_WEB(mWeb);
	//dlog("BContinuousParameter::FlattenedSize()");
	size_t total = BParameter::FlattenedSize();
	if (total > 0)
		total += 6*4;
	return total;
}


status_t
BContinuousParameter::Flatten(void *buffer, ssize_t size) const
{
	CHECK_WEB(mWeb);
	//dlog("BContinuousParameter::Flatten()");
	status_t err = BParameter::Flatten(buffer, size);
	if (err == B_OK)
	{
		size_t upsize = BParameter::FlattenedSize();
		buffer = ((char *)buffer)+upsize;
		size -= upsize;
	}
	else return err;
	put_4(buffer, mMinimum);
	put_4(buffer, mMaximum);
	put_4(buffer, mStepping);
	put_4(buffer, mResponse);
	put_4(buffer, mFactor);
	put_4(buffer, mOffset);
	return B_OK;
}

status_t
BContinuousParameter::Unflatten(
	type_code c, 
	const void *buffer, 
	ssize_t size)
{
	CHECK_WEB(mWeb);
	//dlog("BContinuousParameter::Unflatten()");
	status_t err = BParameter::Unflatten(c, buffer, size);
	if (err != B_OK) return err;

	size_t upsize = BParameter::FlattenedSize();
	buffer = ((char *)buffer)+upsize;
	size -= upsize;
	if (size < 6*4) return B_BAD_VALUE;
	get_4(buffer, size, mMinimum);
	if (SwapOnUnflatten()) mMinimum = B_SWAP_FLOAT(mMinimum);
	get_4(buffer, size, mMaximum);
	if (SwapOnUnflatten()) mMaximum = B_SWAP_FLOAT(mMaximum);
	get_4(buffer, size, mStepping);
	if (SwapOnUnflatten()) mStepping = B_SWAP_FLOAT(mStepping);
	get_4(buffer, size, mResponse);
	if (SwapOnUnflatten()) mResponse = (response)B_SWAP_INT32(mResponse);
	get_4(buffer, size, mFactor);
	if (SwapOnUnflatten()) mFactor = B_SWAP_FLOAT(mFactor);
	get_4(buffer, size, mOffset);
	if (SwapOnUnflatten()) mOffset = B_SWAP_FLOAT(mOffset);
	return B_OK;
}


BContinuousParameter::BContinuousParameter(
	int32 id,
	media_type m_type,
	BParameterWeb * web,
	const char * name,
	const char * kind,
	const char * unit,
	float minimum,
	float maximum,
	float stepping) :
	BParameter(id, m_type, BParameter::B_CONTINUOUS_PARAMETER, web, name, kind, unit)
{
	CHECK_WEB(mWeb);
	mMinimum = minimum;
	mMaximum = maximum;
	mStepping = stepping;
	mResponse = B_LINEAR;
	mFactor = 1;
	mOffset = 0;
	DATA_NAME(BContinuousParameter)
}


BContinuousParameter::~BContinuousParameter()
{
	CHECK_WEB(mWeb);
	/* do nothing special */
}


		/* Mmmh, stuffing! */
status_t
BContinuousParameter::_Reserved_ContinuousParameter_0(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_1(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_2(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_3(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_4(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_5(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_6(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BContinuousParameter::_Reserved_ContinuousParameter_7(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}




#pragma mark --- DiscreteParameter ---



type_code
BDiscreteParameter::ValueType()
{
	CHECK_WEB(mWeb);
	return B_INT32_TYPE;
}


int32
BDiscreteParameter::CountItems()
{
	CHECK_WEB(mWeb);
	return mSelections->CountItems();
}

const char *
BDiscreteParameter::ItemNameAt(
	int32 index)
{
	CHECK_WEB(mWeb);
	return (const char *)mSelections->ItemAt(index);
}


int32
BDiscreteParameter::ItemValueAt(
	int32 index)
{
	CHECK_WEB(mWeb);
	return (int32)mValues->ItemAt(index);
}


status_t
BDiscreteParameter::AddItem(
	int32 value,
	const char * name)
{
	CHECK_WEB(mWeb);
	char * p = strdup(name);
	if (!p) return B_NO_MEMORY;
	if (!mSelections->AddItem(p)) {
		free(p); return B_ERROR;
	}
	if (!mValues->AddItem((void*)value)) {
		free(p); mSelections->RemoveItem(p); return B_ERROR;
	}
	return B_OK;
}



status_t
BDiscreteParameter::MakeItemsFromInputs()
{
	CHECK_WEB(mWeb);
	status_t err;
	for (int ix=0; ix<CountInputs(); ix++) {
		err = AddItem(ix, InputAt(ix)->Name());
		if (err < 0) return err;
	}
	return B_OK;
}


status_t
BDiscreteParameter::MakeItemsFromOutputs()
{
	CHECK_WEB(mWeb);
	status_t err;
	for (int ix=0; ix<CountInputs(); ix++) {
		err = AddItem(ix, OutputAt(ix)->Name());
		if (err < 0) return err;
	}
	return B_OK;
}


void
BDiscreteParameter::MakeEmpty()
{
	CHECK_WEB(mWeb);
	for (int ix=0; ix<mSelections->CountItems(); ix++)
		free(mSelections->ItemAt(ix));
	mSelections->MakeEmpty();
	mValues->MakeEmpty();
}


ssize_t
BDiscreteParameter::FlattenedSize() const
{
	CHECK_WEB(mWeb);
	//dlog("BDiscreteParameter::FlattenedSize()");
	size_t total = BParameter::FlattenedSize();
	total += 4;
	for (int ix=0; ix<mSelections->CountItems(); ix++)
	{
		const char * str = (const char *)mSelections->ItemAt(ix);
		total += (str ? strlen(str) : 0) + 1;
		total += 4;
	}
	return total;
}


status_t
BDiscreteParameter::Flatten(void *buffer, ssize_t size) const
{
	CHECK_WEB(mWeb);
	//dlog("BDiscreteParameter::Flatten()");
	size_t upsize = BParameter::FlattenedSize();
	size_t mysize = FlattenedSize();
	if (size < mysize) return B_NO_MEMORY;
	status_t err = BParameter::Flatten(buffer, size);
	if (err < B_OK) return err;
	buffer = ((char *)buffer)+upsize;
	int32 temp = mSelections->CountItems();
	put_4(buffer, temp);
	for (int ix=0; ix<mSelections->CountItems(); ix++)
	{
		uchar l;
		const char * str = (const char *)mSelections->ItemAt(ix);
		l = (str ? strlen(str) : 0);
		put_1(buffer, l);
		put_N(buffer, str, l);
		temp = (int32)mValues->ItemAt(ix);
		put_4(buffer, temp);
		//dlog("Item(%d, %s)", temp, str ? str : "NULL");
	}
	return B_OK;
}



status_t
BDiscreteParameter::Unflatten(
	type_code c, 
	const void *buffer, 
	ssize_t size)
{
	CHECK_WEB(mWeb);
	//dlog("BDiscreteParameter::Unflatten()");
	status_t err = BParameter::Unflatten(c, buffer, size);
	if (err != B_OK) return err;
	size_t upsize = BParameter::FlattenedSize();
	buffer = ((char *)buffer)+upsize;
	size -= upsize;
	int32 count;
	get_4(buffer, size, count);
	if (SwapOnUnflatten()) count = B_SWAP_INT32(count);
	for (int ix=0; ix<count; ix++)
	{
		char * str;
		get_str(buffer, size, &str);
		mSelections->AddItem(str);
		int32 temp;
		get_4(buffer, size, temp);
		if (SwapOnUnflatten()) temp = B_SWAP_INT32(temp);
		mValues->AddItem((void *)temp);
		//dlog("Item(%d, %s)", temp, str ? str : "NULL");
	}
	return B_OK;
}


BDiscreteParameter::BDiscreteParameter(
	int32 id,
	media_type m_type,
	BParameterWeb * web,
	const char * name,
	const char * kind) :
	BParameter(id, m_type, BParameter::B_DISCRETE_PARAMETER, web, name, kind, NULL)
{
	CHECK_WEB(mWeb);
	mSelections = new BList;
	mValues = new BList;
	DATA_NAME(BDiscreteParameter)
}


BDiscreteParameter::~BDiscreteParameter()
{
	CHECK_WEB(mWeb);
	for (int ix=0; ix<mSelections->CountItems(); ix++)
		free(mSelections->ItemAt(ix));
	delete mSelections;
	delete mValues;
}



		/* Mmmh, stuffing! */
status_t
BDiscreteParameter::_Reserved_DiscreteParameter_0(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_1(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_2(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_3(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_4(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_5(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_6(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BDiscreteParameter::_Reserved_DiscreteParameter_7(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}





#pragma mark --- NullParameter ---



type_code
BNullParameter::ValueType()
{
	CHECK_WEB(mWeb);
	return 0;	/* no value */
}


ssize_t
BNullParameter::FlattenedSize() const
{
	CHECK_WEB(mWeb);
	//dlog("BNullParameter::FlattenedSize()");
	size_t total = BParameter::FlattenedSize();
	return total;
}


status_t
BNullParameter::Flatten(void *buffer, ssize_t size) const
{
	CHECK_WEB(mWeb);
	//dlog("BNullParameter::Flatten()");
	size_t total = FlattenedSize();
	if (total > size) return B_NO_MEMORY;
	status_t err = BParameter::Flatten(buffer, size);
	return err;
}


status_t
BNullParameter::Unflatten(type_code c, const void *buf, ssize_t size)
{
	CHECK_WEB(mWeb);
	//dlog("BNullParameter::Unflatten()");
	status_t err = BParameter::Unflatten(c, buf, size);
	if (err < B_OK) return err;
	size_t size2 = BParameter::FlattenedSize();
	return B_OK;
}


BNullParameter::BNullParameter(
	int32 id,
	media_type m_type,
	BParameterWeb * web,
	const char * name,
	const char * kind) :
	BParameter(id, m_type, BParameter::B_NULL_PARAMETER, web, name, kind, NULL)
{
	CHECK_WEB(mWeb);
	DATA_NAME(BNullParameter)
}


BNullParameter::~BNullParameter()
{
	CHECK_WEB(mWeb);
}



		/* Mmmh, stuffing! */
status_t
BNullParameter::_Reserved_NullParameter_0(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_1(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_2(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_3(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_4(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_5(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_6(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}

status_t
BNullParameter::_Reserved_NullParameter_7(void *)
{
	CHECK_WEB(mWeb);
	return B_ERROR;
}


#if DEBUG
/* force out-lining */
static void _check_web(const BParameterWeb *w, const void *that)
{
	sLock.Lock();
	if (!sWebs.HasItem((void*)w)) {
		fprintf(stderr, "%x %x\n", that, w);
		debugger("Bad web found!");
	}
	sLock.Unlock();
	free(malloc(1));
}

static void _check_class(const char * arg, const char * cls)
{
	while (isdigit(*cls)) cls++;
	assert(!strcmp(arg, cls));
}

#endif
