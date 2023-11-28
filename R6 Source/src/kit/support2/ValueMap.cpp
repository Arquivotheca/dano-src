#include <support2_p/ValueMap.h>

#include <support2/Parcel.h>
#include <support2/MemoryStore.h>
#include <support2/PositionIO.h>

#include <stdio.h>

namespace B {
namespace Support2 {

ssize_t BValueMap::FlattenedSize() const
{
	return sizeof(value_map_header)
		+ sizeof(data_chunk)*CountMaps() + m_dataSize
		+ sizeof(chunk_header);
}

ssize_t BValueMap::Flatten(void *buffer, ssize_t size, BParcel* offsets) const
{
/*
	// Dianne: this version triggers your BAtom debugging code (falsely, I believe)
	IByteOutput::arg io(new BPositionIO(
		atom_ptr<BMemoryStore>(new BMemoryStore(buffer, size))));
*/
	atom_ptr<BMemoryStore> ptr1(new BMemoryStore(buffer, size));
	IByteOutput::arg io(new BPositionIO(ptr1));
	return Flatten(io, offsets);
}

static uint8 drain_buf[256];

static ssize_t write_align(IByteOutput::arg stream, size_t init)
{
	static const uint8 pad[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	const ssize_t rem = chunk_align(init)-init;
	ssize_t writ;
	if (rem > 0) writ = stream->Write(pad, rem);
	else writ = 0;
	return (writ != rem ? (writ < 0 ? writ : B_ERROR) : writ);
}

static ssize_t read_align(IByteInput::arg stream, size_t init)
{
	const ssize_t rem = chunk_align(init)-init;
	ssize_t red;
	if (rem > 0) red = stream->Read(drain_buf, rem);
	else red = 0;
	return (red != rem ? (red < 0 ? red : B_ERROR) : red);
}

ssize_t BValueMap::Flatten(IByteOutput::arg stream, BParcel* offsets) const
{
	AssertEditing();
	
	static const chunk_header end = { END_CODE, 0, sizeof(chunk_header) };
	
	value_map_header head;
	data_chunk data;
	
	const int32 N = CountMaps();
	ssize_t total = 0;
	ssize_t size;
	int32 i;
	
	const size_t base = offsets ? offsets->Base() : 0;
	
	head.signature = VALUE_MAP_SIGNATURE;
	head.size = FlattenedSize();
	head.what = 0;
	head.reserved0 = 0;
	data.header.code = DATA_CODE;
	data.header.reserved = 0;
	
	if ((size=stream->Write(&head, sizeof(head))) != static_cast<ssize_t>(sizeof(head))) {
		return size < B_OK ? size : B_ERROR;
	}
	total += sizeof(head);
	
	for (i=0; i<N; i++) {
		const pair& p = MapAt(i);
		data.key.type = p.key.Type();
		data.key.length = p.key.DataFlattenedSize();
		data.value.type = p.value.Type();
		data.value.length = p.value.DataFlattenedSize();
		data.header.size	= sizeof(data)
							+ chunk_align(data.key.length)
							+ chunk_align(data.value.length);
							
		if (data.key.type == B_BINDER_TYPE || data.key.type == B_BINDER_HANDLE_TYPE) {
			if (offsets) {
				offsets->SetBase(base + total + sizeof(data.header));
				size = offsets->AddBinder(0, sizeof(data));
			} else {
				debugger("Can't flatten BValue containing IBinder objects in to raw data.");
				size = B_ERROR;
			}
		} else if (data.key.type == B_ATOM_TYPE || data.key.type == B_ATOMREF_TYPE) {
			debugger(	"Can't flatten BValue containing BAtom pointers "
						"(maybe you should have added binder objects instead).");
		}
		if (size < B_OK) return size;
		if (data.value.type == B_BINDER_TYPE || data.value.type == B_BINDER_HANDLE_TYPE) {
			if (offsets) {
				offsets->SetBase(base + total);
				size = offsets->AddBinder(
					sizeof(data.header)+sizeof(data.key),
					sizeof(data)+chunk_align(data.key.length));
			} else {
				debugger("Can't flatten BValue containing IBinder objects in to raw data.");
				size = B_ERROR;
			}
		} else if (data.value.type == B_ATOM_TYPE || data.value.type == B_ATOMREF_TYPE) {
			debugger(	"Can't flatten BValue containing BAtom pointers "
						"(maybe you should have added binder objects instead).");
		}
		if (size < B_OK) return size;
		
		if ((size=stream->Write(&data, sizeof(data))) != sizeof(data)) {
			return size < B_OK ? size : B_ERROR;
		}
		
		if (offsets) offsets->SetBase(base+total+sizeof(data));
		if ((size=p.key.DataFlatten(stream, offsets)) != static_cast<ssize_t>(data.key.length)) {
			return size < B_OK ? size : B_ERROR;
		}
		if ((size=write_align(stream, data.key.length)) < B_OK) {
			return size;
		}
		
		if (offsets) offsets->SetBase(base+total+sizeof(data)+chunk_align(data.key.length));
		if ((size=p.value.DataFlatten(stream, offsets)) != static_cast<ssize_t>(data.value.length)) {
			return size < B_OK ? size : B_ERROR;
		}
		if ((size=write_align(stream, data.value.length)) < B_OK) {
			return size;
		}
		
		total += data.header.size;
	}
	
	if ((size=stream->Write(&end, sizeof(end))) != static_cast<ssize_t>(sizeof(end))) {
		return size < B_OK ? size : B_ERROR;
	}

#if DEBUG
	if ((total+sizeof(end)) != head.size) {
		debugger("Header size wrong!");
	}
#endif

	if (offsets) offsets->SetBase(base);
	return total + sizeof(end);
}

ssize_t BValueMap::Unflatten(const void *buffer, size_t avail)
{
	AssertEditing();
	
	atom_ptr<BMemoryStore> mem(new BMemoryStore(const_cast<void*>(buffer), avail));
	return Unflatten(new BPositionIO(mem));
}

ssize_t BValueMap::Unflatten(IByteInput::arg stream)
{
	AssertEditing();
	
	value_map_header head;
	data_chunk data;
	
	pair item;
	ssize_t size;
	ssize_t avail;
	
	m_map.MakeEmpty();
	
	if ((size=stream->Read(&head, sizeof(head))) != static_cast<ssize_t>(sizeof(head))) {
		return size < B_OK ? size : B_ERROR;
	}
	
	if (head.signature != VALUE_MAP_SIGNATURE) {
#warning BValueMap::Unflatten() need to implement byte swapping
		return B_NOT_A_MESSAGE;
	}
	avail = head.size-sizeof(head);
	
	while (avail >= static_cast<ssize_t>(sizeof(data.header))) {
		if ((size=stream->Read(&data.header, sizeof(data.header)))
				!= static_cast<ssize_t>(sizeof(data.header))) {
			return size < B_OK ? size : B_ERROR;
		}
		avail -= data.header.size;
		
		if (avail >= 0) {
			if (data.header.code == END_CODE) {
				return (avail == 0 ? head.size : B_NOT_A_MESSAGE);
			
			} else if (data.header.code == DATA_CODE) {
				if ((size=stream->Read(&data.header+1, sizeof(data)-sizeof(data.header)))
						!= static_cast<ssize_t>(sizeof(data)-sizeof(data.header))) {
					return size < B_OK ? size : B_ERROR;
				}
				if ((size=item.key.DataUnflatten(data.key.type, stream, data.key.length))
						!= static_cast<ssize_t>(data.key.length)) {
					return size < B_OK ? size : B_ERROR;
				}
				if ((size=read_align(stream, data.key.length)) < B_OK) return size;
				if ((size=item.value.DataUnflatten(data.value.type, stream, data.value.length))
						!= static_cast<ssize_t>(data.value.length)) {
					return size < B_OK ? size : B_ERROR;
				}
				if ((size=read_align(stream, data.value.length)) < B_OK) return size;
				if ((size=m_map.AddItem(item)) < B_OK) return size;
			
			} else {
				ssize_t rem = data.header.size - sizeof(data.header);
				while (rem > 0) {
					if (rem > static_cast<ssize_t>(sizeof(drain_buf))) {
						if ((size=stream->Read(drain_buf, sizeof(drain_buf)))
								!= static_cast<ssize_t>(sizeof(drain_buf))) {
							return size < B_OK ? size : B_ERROR;
						}
						rem -= sizeof(drain_buf);
					} else {
						if ((size=stream->Read(drain_buf, rem)) != rem) {
							return size < B_OK ? size : B_ERROR;
						}
						rem = 0;
					}
				}
			}
		}
	}
	
	return B_NOT_A_MESSAGE;
}

status_t BValueMap::SetFirstMap(const BValue& key, const BValue& value)
{
	ssize_t result = m_map.AddItem(pair(key, value));
	if (result >= B_OK) {
		m_dataSize = chunk_align(key.DataFlattenedSize())
					+ chunk_align(value.DataFlattenedSize());
		result = B_OK;
	}
	return result;
}
			
ssize_t BValueMap::OverlayMap(const BValue& key, const BValue& value)
{
	AssertEditing();
	
	if (value.IsDefined() && key.IsDefined() && (!value.IsNull() || !key.IsNull())) {
		const pair p(key, value);
		ssize_t index;
		if (GetIndexOf(p, reinterpret_cast<size_t*>(&index))) {
			pair& p = m_map.EditItemAt(index);
			m_dataSize += (chunk_align(value.DataFlattenedSize())
						- chunk_align(p.value.DataFlattenedSize()));
			p.value = value;
		} else {
			index = m_map.AddItemAt(p, index);
			if (index >= B_OK)
				m_dataSize += chunk_align(key.DataFlattenedSize())
							+ chunk_align(value.DataFlattenedSize());
		}
		return index;
	}
	return B_BAD_VALUE;
}

ssize_t BValueMap::OverlayMap(const value_ref& key, const BValue& value)
{
	AssertEditing();
	
	if (value.IsDefined() && key.type != 0 && (!value.IsNull() || key.type != B_NULL_TYPE)) {
		ssize_t index;
		if (GetIndexOf(key, value_ref(value), reinterpret_cast<size_t*>(&index))) {
			pair& p = m_map.EditItemAt(index);
			m_dataSize += (chunk_align(value.DataFlattenedSize())
						- chunk_align(p.value.DataFlattenedSize()));
			p.value = value;
		} else {
			const pair p(key, value);
			index = m_map.AddItemAt(p, index);
			if (index >= B_OK)
				m_dataSize += chunk_align(p.key.DataFlattenedSize())
							+ chunk_align(p.value.DataFlattenedSize());
		}
		return index;
	}
	return B_BAD_VALUE;
}

ssize_t BValueMap::InheritMap(const BValue& key, const BValue& value)
{
	AssertEditing();
	
	if (value.IsDefined() && key.IsDefined() && (!value.IsNull() || !key.IsNull())) {
		const pair p(key, value);
		ssize_t index;
		if (!GetIndexOf(p, reinterpret_cast<size_t*>(&index))) {
			index = m_map.AddItemAt(p, index);
			if (index >= B_OK)
				m_dataSize += chunk_align(key.DataFlattenedSize())
							+ chunk_align(value.DataFlattenedSize());
		}
		return index;
	}
	return B_BAD_VALUE;
}

ssize_t BValueMap::InheritMap(const value_ref& key, const BValue& value)
{
	AssertEditing();
	
	if (value.IsDefined() && key.type != 0 && (!value.IsNull() || key.type != B_NULL_TYPE)) {
		ssize_t index;
		if (!GetIndexOf(key, value_ref(value), reinterpret_cast<size_t*>(&index))) {
			const pair p(key, value);
			index = m_map.AddItemAt(p, index);
			if (index >= B_OK)
				m_dataSize += chunk_align(p.key.DataFlattenedSize())
							+ chunk_align(p.value.DataFlattenedSize());
		}
		return index;
	}
	return B_BAD_VALUE;
}

ssize_t BValueMap::InheritMap(const value_ref& key, const value_ref& value)
{
	AssertEditing();
	
	if (value.type != 0 && key.type != 0 && (value.type != B_NULL_TYPE || key.type != B_NULL_TYPE)) {
		ssize_t index;
		if (!GetIndexOf(key, value, reinterpret_cast<size_t*>(&index))) {
			const pair p(key, value);
			index = m_map.AddItemAt(p, index);
			if (index >= B_OK)
				m_dataSize += chunk_align(p.key.DataFlattenedSize())
							+ chunk_align(p.value.DataFlattenedSize());
		}
		return index;
	}
	return B_BAD_VALUE;
}

status_t BValueMap::RemoveMap(const value_ref& key, const value_ref& value)
{
	AssertEditing();
	
	if (key != value_ref::null || value != value_ref::null) {
		size_t index;
		if (GetIndexOf(key, value, &index)) {
			RemoveMapAt(index);
			return B_OK;
		}
	} else {
		m_map.MakeEmpty();
		m_dataSize = 0;
	}
	
	return B_NAME_NOT_FOUND;
}

status_t BValueMap::RenameMap(const value_ref& old_key, const BValue& new_key)
{
	AssertEditing();
	
	if (old_key.type != B_NULL_TYPE && old_key.type != 0 && new_key.IsSpecified()) {
		ssize_t old_index, new_index;
		if (GetIndexOf(old_key, value_ref::null, reinterpret_cast<size_t*>(&old_index))) {
			pair tmp = m_map.ItemAt(old_index);
			const size_t oldSize = tmp.key.DataFlattenedSize();
			tmp.key = new_key;
			if (!GetIndexOf(tmp, reinterpret_cast<size_t*>(&new_index))) {
				m_map.RemoveItemsAt(old_index);
				const ssize_t error =
					m_map.AddItemAt(tmp, new_index > old_index ? new_index-1 : new_index);
				if (error >= B_OK) {
					m_dataSize += chunk_align(tmp.key.DataFlattenedSize())
								- chunk_align(oldSize);
				}
				return error < B_OK ? error : B_OK;
			}
			return B_NAME_IN_USE;
		}
		return B_NAME_NOT_FOUND;
	}
	return B_BAD_VALUE;
}

int32 BValueMap::Compare(const BValueMap& o) const
{
	const int32 N1 = CountMaps();
	const int32 N2 = o.CountMaps();
	const int32 N = N1 < N2 ? N1 : N2;
	int32 c;
	for (int32 i=0; i<N; i++) {
		const pair& p1 = MapAt(i);
		const pair& p2 = o.MapAt(i);
		c = p1.key.Compare(p2.key);
		if (c != 0) return c;
		c = p1.value.Compare(p2.value);
		if (c != 0) return c;
	}
	return N1 < N2 ? -1 : (N2 < N1 ? 1 : 0);
}

int32 BValueMap::FastCompare(const BValueMap& o) const
{
	const int32 N1 = CountMaps();
	const int32 N2 = o.CountMaps();
	const int32 N = N1 < N2 ? N1 : N2;
	int32 c;
	for (int32 i=0; i<N; i++) {
		const pair& p1 = MapAt(i);
		const pair& p2 = o.MapAt(i);
		c = p1.key.FastCompare(p2.key);
		if (c != 0) return c;
		c = p1.value.FastCompare(p2.value);
		if (c != 0) return c;
	}
	return N1 < N2 ? -1 : (N2 < N1 ? 1 : 0);
}

BValue *BValueMap::BeginEditMapAt(size_t index)
{
	if (m_editIndex >= 0)
		debugger("Can not edit more than one item at a time");
		
	pair& p(m_map.EditItemAt(index));
	m_editIndex = index;
	m_editSize = chunk_align(p.value.DataFlattenedSize());

	return &p.value;
}

void BValueMap::EndEditMapAt()
{
	if (m_editIndex < 0)
		debugger("Value not being edited!");

	pair& p(m_map.EditItemAt(m_editIndex));
	m_editIndex = -1;
	
	m_dataSize += chunk_align(p.value.DataFlattenedSize()) - m_editSize;
}

bool BValueMap::GetIndexOf(const value_ref& k, const value_ref& v, size_t* index) const
{
	ssize_t mid, low = 0, high = m_map.CountItems()-1;
	const pair* items = &m_map.ItemAt(0);
	while (low <= high) {
		mid = (low + high)/2;
		const int32 cmp = items[mid].compare(k, v);
		if (cmp > 0) {
			high = mid-1;
		} else if (cmp < 0) {
			low = mid+1;
		} else {
			*index = mid;
			return true;
		}
	}
	
	*index = low;
	return false;
}

bool BValueMap::GetIndexOf(const pair& o, size_t* index) const
{
	ssize_t mid, low = 0, high = m_map.CountItems()-1;
	const pair* items = &m_map.ItemAt(0);
	while (low <= high) {
		mid = (low + high)/2;
		const int32 cmp = items[mid].compare(o);
		if (cmp > 0) {
			high = mid-1;
		} else if (cmp < 0) {
			low = mid+1;
		} else {
			*index = mid;
			return true;
		}
	}
	
	*index = low;
	return false;
}


} }	// namespace B::Support2
