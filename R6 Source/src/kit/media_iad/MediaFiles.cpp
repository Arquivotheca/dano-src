/*	MediaFiles.cpp	*/

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <MediaFiles.h>
#include <trinity_p.h>
#include <tr_debug.h>
#include <Autolock.h>


#define THAT ((_BMediaRosterP *)BMediaRoster::CurrentRoster())
#define SET_STRING(s,v) s->SetTo(v)
#define ASSIGN_STRING(s,v) s.SetTo(v)
#define COMPARE_STRING(s,v) (s == v)


const char BMediaFiles::B_SOUNDS[] = "Sounds";

static BLocker mediaFilesLocker("MediaFiles");

BMediaFiles::BMediaFiles()
{
	m_type_index = 0;
	m_item_index = 0;
}

BMediaFiles::~BMediaFiles()
{
	for (int ix=0; ix<m_types.CountItems(); ix++) {
		free(m_types.ItemAt(ix));
	}
	m_types.MakeEmpty();
	for (int ix=0; ix<m_items.CountItems(); ix++) {
		delete (_BMediaRosterP::media_item *)m_items.ItemAt(ix);
	}
	m_items.MakeEmpty();
}


status_t 
BMediaFiles::RewindTypes()
{
	BAutolock lock(mediaFilesLocker);
	for (int ix=0; ix<m_types.CountItems(); ix++) {
		free(m_types.ItemAt(ix));
	}
	m_types.MakeEmpty();
	m_type_index = 0;
	return THAT->GetMediaTypes(&m_types);
}

status_t 
BMediaFiles::GetNextType(
	BString * out_type)
{
	if (!out_type) return B_BAD_VALUE;
	BAutolock lock(mediaFilesLocker);
	if (m_type_index >= m_types.CountItems()) {
		return B_BAD_INDEX;
	}
	SET_STRING(out_type, (const char *)m_types.ItemAt(m_type_index));
	m_type_index += 1;
	return B_OK;
}


status_t 
BMediaFiles::RewindRefs(
	const char * type)
{
//	fprintf(stderr, "RewindRefs() one\n");
	if (!type) return B_BAD_VALUE;
//	fprintf(stderr, "RewindRefs() two\n");
	BAutolock lock(mediaFilesLocker);
	for (int ix=0; ix<m_items.CountItems(); ix++) {
		delete (_BMediaRosterP::media_item *)m_items.ItemAt(ix);
	}
//	fprintf(stderr, "RewindRefs() three\n");
	m_items.MakeEmpty();
	m_item_index = 0;
//	fprintf(stderr, "RewindRefs() four\n");
	ASSIGN_STRING(m_cur_type, type);
//	fprintf(stderr, "RewindRefs() five\n");
	return THAT->GetTypeItems(type, &m_items);
}

status_t 
BMediaFiles::GetNextRef(
	BString * out_type, 
	entry_ref * out_ref)
{
	if (!out_type) return B_BAD_VALUE;
//	fprintf(stderr, "m_item_index %d  CountItems() %d\n", m_item_index, m_items.CountItems());
	BAutolock lock(mediaFilesLocker);
	if (m_item_index >= m_items.CountItems()) {
		return B_BAD_INDEX;
	}
	_BMediaRosterP::media_item * item = (_BMediaRosterP::media_item *)m_items.ItemAt(m_item_index);
	m_item_index += 1;
	SET_STRING(out_type, item->name);
	if (out_ref) *out_ref = item->ref;
//	fprintf(stderr, "returning %d: %s, %s\n", m_item_index, item->name, item->ref.name);
	return B_OK;
}


status_t 
BMediaFiles::GetRefFor(
	const char * type,
	const char * item,
	entry_ref * out_ref)
{
	if (!type || !item || !out_ref) return B_BAD_VALUE;
	dlog("GetRefFor(%s, %s)", type, item);
	BAutolock lock(mediaFilesLocker);
	if (!COMPARE_STRING(m_cur_type, type)) {
		status_t err = RewindRefs(type);
		if (err < B_OK) return err;
	}
	for (int ix=0; ix<m_items.CountItems(); ix++) {
		_BMediaRosterP::media_item * it = (_BMediaRosterP::media_item *)m_items.ItemAt(ix);
		if (!strcasecmp(it->name, item)) {
			*out_ref = it->ref;
			dlog("-> %s, %s", it->name, it->ref.name);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

status_t 
BMediaFiles::SetRefFor(
	const char * type,
	const char * item,
	const entry_ref & ref)
{
	BAutolock lock(mediaFilesLocker);
	status_t err = THAT->SetTypeItem(type, item, ref);
//	fprintf(stderr, "SetRefFor(%s, %s, %s)\n", type, item, ref.name);
	if ((err >= B_OK) && COMPARE_STRING(m_cur_type, type)) {
		for (int ix=0; ix<m_items.CountItems(); ix++) {
			_BMediaRosterP::media_item * it = (_BMediaRosterP::media_item *)m_items.ItemAt(ix);
			if (!strcasecmp(it->name, item)) {
				it->ref = ref;
				return B_OK;
			}
		}
	}
	return err;
}

status_t 
BMediaFiles::RemoveRefFor(
	const char * type,
	const char * item,
	const entry_ref & ref)
{
	BAutolock lock(mediaFilesLocker);
	status_t err = THAT->RemoveTypeItemRef(type, item, ref);
//	fprintf(stderr, "RemoveRefFor(%s, %s, %s)\n", type, item, ref.name);
	if ((err >= B_OK) && COMPARE_STRING(m_cur_type, type)) {
		for (int ix=0; ix<m_items.CountItems(); ix++) {
			_BMediaRosterP::media_item * it = (_BMediaRosterP::media_item *)m_items.ItemAt(ix);
			if (!strcasecmp(it->name, item) && (it->ref == ref)) {
				it->ref.name[0] = 0;
				it->ref.device = -1;
				it->ref.directory = -1LL;
				break;
			}
		}
	}
	return err;
}

status_t 
BMediaFiles::RemoveItem(
	const char * type,
	const char * item)
{
	BAutolock lock(mediaFilesLocker);
	status_t err = THAT->RemoveTypeItem(type, item);
//	fprintf(stderr, "RemoveRefFor(%s, %s, %s)\n", type, item, ref.name);
	if ((err >= B_OK) && COMPARE_STRING(m_cur_type, type)) {
		for (int ix=0; ix<m_items.CountItems(); ix++) {
			_BMediaRosterP::media_item * it = (_BMediaRosterP::media_item *)m_items.ItemAt(ix);
			if (!strcasecmp(it->name, item)) {
				delete it;
				m_items.RemoveItem(ix);
				return B_OK;
			}
		}
	}
	return err;
}


status_t BMediaFiles::_Reserved_MediaFiles_0(void * type, ...)	//	RemoveItem
{
	va_list vl;
	va_start(vl, type);
	const char * item = va_arg(vl, const char *);
	status_t err = RemoveItem((const char *)type, item);
	va_end(vl);
	return err;
}


status_t BMediaFiles::_Reserved_MediaFiles_1(void *, ...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_2(void *, ...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_3(void *, ...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_4(void *, ...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_5(void *, ...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_6(void *, ...) { return B_ERROR; }
status_t BMediaFiles::_Reserved_MediaFiles_7(void *, ...) { return B_ERROR; }
