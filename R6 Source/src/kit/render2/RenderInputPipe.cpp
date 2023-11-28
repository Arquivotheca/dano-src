/***************************************************************************
//
//	File:			render2/BRenderInputPipe.cpp
//
//	Description:	Abstract drawing interface.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/


#include <support2/SupportDefs.h>
#include <support2/StdIO.h>
#include <render2/Render.h>
#include <render2_p/RenderInputPipe.h>
#include <render2_p/RenderProtocol.h>


namespace B {
namespace Render2 {

using namespace Support2;
using namespace Raster2;

#define DEBUG_BUFFER	1

BRenderInputPipe::BRenderInputPipe()
	:	m_cmdBuffer(NULL),
		m_cmdPtr(NULL),
		m_cmdLookAheadPtr(NULL),
		m_dataBuffer(NULL),
		m_dataPtr(NULL),
	 	m_datSize(0),
	 	m_cmdSize(0),
	 	m_lookAheadIndex(0)
{
}

BRenderInputPipe::~BRenderInputPipe()
{
	PreloadIfAvaillable();
	if (m_opcodes.CountItems()) {
		#if DEBUG_BUFFER
			debugger("leaking command buffers!");
		#endif
		while (m_opcodes.CountItems()) {
			delete m_opcodes.ItemAt(0);
			m_opcodes.RemoveItemsAt(0);
		}
	}
	if (m_data.CountItems()) {
		#if DEBUG_BUFFER
			debugger("leaking data buffers!");
		#endif
		while (m_data.CountItems()) {
			delete m_data.ItemAt(0);
			m_data.RemoveItemsAt(0);
		}
	}
}

// *****************************************************************
// #pragma mark -

ssize_t BRenderInputPipe::readptr(uint8 const ** const pvalue, size_t size)
{
	size_t s = size_dat();
	s = (s < size) ? s : size;
	if ((s == 0) && (size != 0)) {
		if (next_dat() != B_OK) {
			*pvalue = m_dataPtr;
			return 0;
		} else {
			s = size_dat();
			s = (s < size) ? s : size;
		}
	}
	*pvalue = m_dataPtr;
	m_dataPtr += s;
	return s;
}

// *****************************************************************
// #pragma mark -

status_t BRenderInputPipe::next_dat()
{
	if (m_data.CountItems()) {
		delete m_data.ItemAt(0);
		m_data.RemoveItemsAt(0);
		if (m_data.CountItems()) {
			m_dataBuffer = static_cast<const uint8 *>(m_data.ItemAt(0)->Data());
			m_datSize = m_data.ItemAt(0)->Length();
			m_dataPtr = m_dataBuffer;
			return B_OK;
		}
	}
	m_dataBuffer = NULL;
	m_datSize = 0;
	m_dataPtr = NULL;
	return B_ERROR;
}

status_t BRenderInputPipe::next_cmd()
{
	if (m_opcodes.CountItems()) {
		delete m_opcodes.ItemAt(0);
		m_opcodes.RemoveItemsAt(0);
		if (m_opcodes.CountItems()) {
			m_cmdBuffer = static_cast<const uint8 *>(m_opcodes.ItemAt(0)->Data());
			m_cmdSize = m_opcodes.ItemAt(0)->Length();
			m_cmdPtr = m_cmdBuffer;
			rewindOp();
			return B_OK;
		}
	}
	m_cmdBuffer = NULL;
	m_cmdSize = 0;
	m_cmdPtr = NULL;
	rewindOp();
	return B_ERROR;
}

// *****************************************************************
// #pragma mark -

status_t BRenderInputPipe::Receive(		uint32 code,
										BParcel& data,
										BParcel* reply,
										uint32 flags)
{
	// bout << "Transact" << endl;
	if (code == B_RENDER_TRANSACTION_CMDS) {
		BParcel *item = new BParcel();
		item->Transfer(&data);
		m_opcodes.AddItem(item);
//bout << "B_RENDER_TRANSACTION_CMDS" << endl << indent << BHexDump(item->Data(), item->Length()) << dedent << endl;
		if (m_cmdBuffer == NULL) {
			m_cmdBuffer = static_cast<const uint8 *>(m_opcodes.ItemAt(0)->Data());
			m_cmdSize = m_opcodes.ItemAt(0)->Length();
			m_cmdPtr = m_cmdBuffer;
		}
	} else if (code == B_RENDER_TRANSACTION_DATA) {
		BParcel *item = new BParcel();
		item->Transfer(&data);
		m_data.AddItem(item);
//bout << "B_RENDER_TRANSACTION_DATA" << endl << indent << BHexDump(item->Data(), item->Length()) << dedent << endl;
		if (m_dataBuffer == NULL) {
			m_dataBuffer = static_cast<const uint8 *>(m_data.ItemAt(0)->Data());
			m_datSize = m_data.ItemAt(0)->Length();
			m_dataPtr = m_dataBuffer;
		}
	}
	return B_OK;
}

} } // namespace B::Render2

