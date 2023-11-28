
/*******************************************************************************
/
/	File:			Cursor.h
/
/   Description:    BCursor describes a view-wide or application-wide cursor
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#include <Debug.h>
#include <Cursor.h>
#include <messages.h>
#include <message_util.h>
#include <token.h>
#include <session.h>

void BCursor::_ReservedCursor1() {};
void BCursor::_ReservedCursor2() {};
void BCursor::_ReservedCursor3() {};
void BCursor::_ReservedCursor4() {};

BCursor::BCursor(const void *cursorData)
{
	m_serverToken = NO_TOKEN;
	m_needToFree = false;
	if (cursorData != NULL) {
		_BAppServerLink_ link;
		link.session->swrite_l(GR_NEW_CURSOR);
		const uint8 *cData = reinterpret_cast<const uint8 *>(cursorData);
		int32 cWidth = (int32)cData[0];
		int32 cHeight = (int32)cData[0];
		int32 cDepth = (int32)cData[1];
		if(!cWidth || !cHeight || !cDepth) {
			cWidth = 16;
			cHeight = 16;
			cDepth = 1;
		}
		
		if(cDepth == 1)
			cWidth = ((cWidth + 7) / 8) * 8;
		
		int32 cSize = cWidth*cHeight*cDepth*2 / 8 + 4;
		link.session->swrite(sizeof(int32),&cSize);
		link.session->swrite(cSize,(void*)cursorData);
		link.session->flush();
		link.session->sread(4,&m_serverToken);
		m_needToFree = true;
	};
}

BCursor::BCursor(BMessage *)
{
	m_serverToken = NO_TOKEN;
	m_needToFree = false;
}

BCursor::~BCursor()
{
	if (m_needToFree && (m_serverToken != NO_TOKEN)) {
		_BAppServerLink_ link(false);
		if (link.session) {
			link.session->swrite_l(GR_DELETE_CURSOR);
			link.session->swrite_l(m_serverToken);
		}
		m_serverToken = NO_TOKEN;
	};
}

status_t BCursor::Archive(BMessage *, bool ) const
{
	return B_OK;
}

BArchivable * BCursor::Instantiate(BMessage *)
{
	return NULL;
}

status_t BCursor::Perform(perform_code , void *)
{
	return B_OK;
}

