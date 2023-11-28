/*	$Id: DShowAddress.cpp,v 1.5 1999/05/03 13:09:52 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 10/30/98 16:58:02
*/

#include "bdb.h"
#include "DShowAddress.h"
#include "DMessages.h"
#include "DStackCrawlWindow.h"
#include "DTeam.h"
#include "DStackCrawl.h"
#include <Message.h>
#include <Messenger.h>

DShowAddress::DShowAddress(BRect frame, const char *name, window_type type, int flags,
	BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	fTeam = NULL;
	FindView("addr")->MakeFocus();
	Show();
} /* DShowAddress::DShowAddress */

void DShowAddress::SetTeam(DTeam *team)
{
	fTeam = team;
} // DShowAddress::SetTeam

bool DShowAddress::OKClicked()
{
	BMessage m(kMsgShowAddress);
	
	char *s = strdup(GetText("addr"));

	try
	{
		char *t = strtok(s, ", ");
		
		std::vector<ptr_t> pcs;
		ptr_t addr;
		
		while (t)
		{
			if (strncmp(t, "0x", 2) == 0)
				t += 2;

			addr = strtoul(t, NULL, 16);
			pcs.push_back(addr);

			t = strtok(NULL, ", ");
		}
		
		std::reverse(pcs.begin(), pcs.end());
		
		if (pcs.size() == 1)
		{
			m.AddInt32("address", addr);
			FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&m,
				(BHandler *)0, 1000));
		}
		else
		{
			DStackCrawl *sc = new DStackCrawl(pcs, fTeam->GetSymWorld());
			new DStackCrawlWindow("Stack Crawl", *fTeam, *sc);
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	free(s);
	return true;
} /* DShowAddress::OKClicked */
