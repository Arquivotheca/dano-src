/***************************************************************************
//
//	File:			render2/RenderOutputPipe.cpp
//
//	Description:	Abstract drawing interface.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/


#include <support2/SupportDefs.h>
#include <support2/StdIO.h>
#include <render2/Render.h>
#include <render2_p/RenderOutputPipe.h>
#include <render2_p/RenderProtocol.h>


namespace B {
namespace Render2 {

using namespace Support2;
using namespace Raster2;


BRenderOutputPipe::BRenderOutputPipe(const IBinder::ptr& remote, size_t data_size, size_t cmd_size)
	: 	m_cmdSize(cmd_size),
	 	m_datSize(data_size),
		m_count(0),
		m_opcode(GRP_NOP),
		m_remote(remote),
		m_isOpen(true)
{
	m_cmdBuffer = new(std::nothrow) uint8[m_cmdSize];
	m_dataBuffer = new(std::nothrow) uint8[m_datSize];
	m_cmdPtr = m_cmdBuffer;
	m_dataPtr = m_dataBuffer;
}

BRenderOutputPipe::~BRenderOutputPipe()
{
	delete [] m_cmdBuffer;
	delete [] m_dataBuffer;
}

status_t BRenderOutputPipe::InitCheck() const
{
	if ((!m_cmdBuffer) || (!m_dataBuffer))
		return B_NO_MEMORY;
	return B_OK;
}

status_t BRenderOutputPipe::Close()
{
	flush_cmd();
	flush_dat();
	m_isOpen = false;
	return B_OK;
}

// *****************************************************************
// #pragma mark -

void BRenderOutputPipe::flush_cmd()
{
	if (m_isOpen == false) {
		debugger("BRenderOutputPipe is closed");
		return;
	}

	writeOp(GRP_NOP); // Used to flush the current command into the stream
	if ((m_cmdPtr-m_cmdBuffer) == 0) { // nothing to flush
		return;
	}
	BParcel command;
	BParcel reply;
	command.Reference(m_cmdBuffer, m_cmdPtr-m_cmdBuffer);
	ssize_t status = Remote()->Transact(B_RENDER_TRANSACTION_CMDS, command, &reply);
	if (status >= B_OK) {
		if (reply.Length() >= static_cast<ssize_t>(sizeof(ssize_t))) {
			status = *static_cast<const ssize_t*>(reply.Data());
		} else {
			status = B_ERROR;
		}
	}
	if (status < B_OK) {
		berr << "error sending the command buffer : " << strerror(status) << endl;
	}
	// reset the command pointer
	m_cmdPtr = m_cmdBuffer;
}

void BRenderOutputPipe::flush_dat()
{
	if (m_isOpen == false) {
		debugger("BRenderOutputPipe is closed");
		return;
	}

	if ((m_dataPtr-m_dataBuffer) == 0) { // nothing to flush
		return;
	}
	// Always flush the command buffer first to be sure
	// we will not need to keep BParcel around
	flush_cmd();
	BParcel reply;
	m_offsets.Reference(m_dataBuffer, m_dataPtr-m_dataBuffer);
	ssize_t status = Remote()->Transact(B_RENDER_TRANSACTION_DATA, m_offsets, &reply);
	if (status >= B_OK) {
		if (reply.Length() >= static_cast<ssize_t>(sizeof(ssize_t))) {
			status = *static_cast<const ssize_t*>(reply.Data());
		} else {
			status = B_ERROR;
		}
	}
	if (status < B_OK) {
		berr << "error sending the data buffer : " << strerror(status) << endl;
	}
	// reset the data pointer
	m_offsets.SetBinderOffsets(NULL, 0);
	m_dataPtr = m_dataBuffer;
}

// *****************************************************************
// #pragma mark -

void BRenderOutputPipe::writeOp(uint8 op)
{
	// writes an opcode with RLE compression
	// never write GRP_NOP
	if (op == m_opcode) {
		if (m_opcode != GRP_NOP) {
			if (m_count == 0xFF) {
				assert_cmd(2);
				*m_cmdPtr++ = m_opcode | GRP_RLE;
				*m_cmdPtr++ = 0xFF;
				m_count = 0;
			} else {
				m_count++;
			}
		}
	} else {
		if (m_count) {
			assert_cmd(2);
			*m_cmdPtr++ = m_opcode | GRP_RLE;
			*m_cmdPtr++ = m_count;
			m_count = 0;
		} else {
			if (m_opcode != GRP_NOP) {
				assert_cmd(1);
				*m_cmdPtr++ = m_opcode;
			}
		}
		m_opcode = op;
	}
}

void BRenderOutputPipe::write(const uint8 *buffer, size_t length)
{
	while (length) {
		if (size_dat() == 0) {
			flush_dat();
		}
		const size_t left = size_dat();		
		const size_t to_send = (left < length) ? left : length;
		memcpy(m_dataPtr, buffer, to_send);
		length -= to_send;
		m_dataPtr += to_send;
		buffer += to_send;
	}
}

void BRenderOutputPipe::write_align(const uint8 *buffer, size_t length)
{
	write(buffer, length);
	m_dataPtr += ((4 - (length & 0x3)) & 0x3);
}

} } // namespace B::Render2

