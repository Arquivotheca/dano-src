#include "RecordModule.h"
#include "MinitelView.h"
#include "Protocole.h"
#include <CheckBox.h>
#include <TextControl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <File.h>
#include <Directory.h>
#include <MenuItem.h>


//========================================================================

RecordModule::RecordModule(BRect frame, const char *title, window_type type, ulong flags, bool free_when_closed)
	: BWindow(frame, title, type, flags, free_when_closed)
{
	saveMode = 0;
	fic_save[0] = '\0';
	vit_nbr = 51;
}

//------------------------------------------------------------------------

void RecordModule::ReInit()
{
	cur_page = 0;
	page[0] = 0;
	max_page = 1;
	page_stop = 0;
//	stop_item->SetValue(page_stop);
	autoDraw = 1;
//	autoDraw_item->SetValue(autoDraw);
	if (saveMode)
	{
		asciiSaveMI->SetMarked(TRUE);
		binSaveMI->SetMarked(FALSE);
	}
	else
	{
		binSaveMI->SetMarked(TRUE);
		asciiSaveMI->SetMarked(FALSE);
	}
	mode = kSTOP;
	affiche_time_entry = 0;
	nbr_data = 0;
	cur_data = 0;
	modif_flag = 0;
	fic_save[0] = '\0';
	max_alloc = kPAS_ALLOC;
	buffer = (unsigned char *)malloc(max_alloc);
	if (buffer == NULL)
	{
//		NXRunAlertPanel(NULL,
//		"L'initialisation du buffer d'enregistrement a ðchouð\nPas d'enregistrement de pages possible",
//		"Ok", NULL, NULL);		
fprintf(stderr,"Error allocating buffer\n");
		Close();
		max_alloc = 0;
	}
	buffer[cur_data++] = 0x13;
	buffer[cur_data++] = 0x0;
}

//------------------------------------------------------------------------

void RecordModule::ChangePageStop()
{
	page_stop = stop_item->Value();
}

//------------------------------------------------------------------------

void RecordModule::ChangeAutoDraw()
{
	autoDraw = autoDraw_item->Value();
}

//------------------------------------------------------------------------

void RecordModule::ChangeVitesse()
{
/*
	LRSlider *sender = (LRSlider *)sender1;
//	vit_nbr = 100 - sender->intValue();
	vit_nbr = 2*sender->intValue()+1;
*/
}

//------------------------------------------------------------------------

void RecordModule::Enreg()
{
	if (buffer == NULL)
		ReInit();
	if (buffer == NULL)
	{
//		NXRunAlertPanel(NULL,
//			"L'enregistrement est impossible",
//		"Ok", NULL, NULL);
		return;
	}
	if (mode != kSTOP)
	{
		cur_data = nbr_data;
	}	
	mode = kENREG;
	modif_flag = 1;
}

//------------------------------------------------------------------------

void RecordModule::DoEnreg(unsigned char c)
{
	unsigned char *ptr;
	
//fprintf(stderr, "RecordModule::DoEnreg() begin\n");
	if (mode == kENREG)
	{
		if (c != 0)
		{
			if (nbr_data >= max_alloc)
			{
				ptr = (unsigned char *)realloc(buffer, max_alloc + kPAS_ALLOC);
				if (ptr == NULL)
				{
//					NXRunAlertPanel(NULL,
//						"Le buffer est plein\nL'enregistrement est stoppð",
//						"Ok", NULL, NULL);
fprintf(stderr,"Buffer reallocation impossible\n");
					mode = kSTOP;
				}
				else
				{
					buffer = ptr;
					max_alloc = max_alloc + kPAS_ALLOC;
					buffer[nbr_data++] = c;
					if (c == 0x13)
					{
						page[max_page++] = nbr_data - 1;
						if (max_page >= kMAX_PAGE)
							max_page = kMAX_PAGE - 1;
						cur_page = max_page - 1;
						item_page->setIntValue(cur_page);
					}
				}
			}
			else
			{
				buffer[nbr_data++] = c;
				if (c == 0x13)
				{
					page[max_page++] = nbr_data - 1;
					if (max_page >= kMAX_PAGE)
						max_page = kMAX_PAGE - 1;
					cur_page = max_page - 1;
					item_page->setIntValue(cur_page);
				}
			}
		}
	}
//fprintf(stderr, "RecordModule::DoEnreg() end\n");
}

//------------------------------------------------------------------------

void RecordModule::StopEnreg()
{
	if (mode == kENREG)
	{
		mode = kSTOP;
	}
}

//------------------------------------------------------------------------

void RecordModule::Pause()
{
	if (mode == kPLAY)
	{
		mode = kSTOP;
		StopPlay();
	}
}

//------------------------------------------------------------------------

void RecordModule::GoTo(int32 num)
{
	if (num == 0)
	{
		ecran->efface();
		cur_data = 0;
		cur_page = 0;
		item_page->setIntValue(cur_page);
		return;
	}

	ecran->setDrawOff();

	if (num <= cur_page)
	{
		cur_data = 0;
		cur_page = 0;
	}

	while (cur_data < page[num]+1)
	{
		if (cur_data >= nbr_data)
		{
			cur_data = 0;
			cur_page = 0;
			item_page->setIntValue(cur_page);
			break;
		}
		if (buffer[cur_data] == 0x13)
		{
			cur_page++;
			if (cur_page >= max_page) cur_page = 0;
			item_page->setIntValue(cur_page);
		}
		protocole->charFromModem(buffer[cur_data++]);
	}
	ecran->setDrawOn();

	if (autoDraw)
	{
		ecran->Window()->Lock();
		ecran->Invalidate();
		ecran->Window()->Unlock();
	}
}

//------------------------------------------------------------------------

void RecordModule::SkipBwd()
{
	int32	pg;

	if ((mode == kPLAY) || (mode == kSTOP))
	{
		pg = cur_page - 1;
		if (pg < 0)
			pg = max_page - 1;
		GoTo(pg);
	}
}

//------------------------------------------------------------------------

void RecordModule::SkipGo()
{
	int32	pg;
	
	if ((mode == kPLAY) || (mode == kSTOP))
	{
		pg = item_page->intValue();
		if (pg >= max_page)
			pg = max_page - 1;
		if (pg < 0)
			pg = 0;
		GoTo(pg);
	}
}

//------------------------------------------------------------------------

void RecordModule::SkipFwd()
{
	int32	pg;
	
	if ((mode == kPLAY) || (mode == kSTOP))
	{
 		pg = cur_page + 1;
		if (pg >= max_page)
			pg = 0;
		GoTo(pg);
	}
}

//------------------------------------------------------------------------

void RecordModule::StopPlay()
{
	if (affiche_time_entry != 0)
	{
//		DPSRemoveTimedEntry(affiche_time_entry);
		Lock();
		contentView->SetFlags(0); // DON'T WORK !!!???
		SetPulseRate(3000000);
		Unlock();
		affiche_time_entry = 0;
	}
}

//------------------------------------------------------------------------

void RecordModule::Pulse()
{
	int32	i;

/*
	for (i = 0; i < 50; i++)
		DoReplay();
*/
	for (i = 0; i < vit_nbr; i++)
		DoReplay();
}

//------------------------------------------------------------------------

void RecordModule::DoReplay()
{	
//fprintf(stderr, "RecordModule::DoReplay() begin\n");
	if (mode != kPLAY) 
	{
		StopPlay();
fprintf(stderr, "RecordModule::DoReplay() end no play\n");
		return;
	}

/*
	if (nv++ < vit_nbr * 5)
	{
		return;
fprintf(stderr, "RecordModule::DoReplay() end no turn\n");
	}
	else
		nv = 0;
*/
		
	if (cur_data >= nbr_data)
	{
		cur_data = 0;
		cur_page = 0;
		item_page->setIntValue(cur_page);
	}
	
	if (buffer[cur_data] == 0x13)
	{
		cur_page++;
		if (cur_page >= max_page)
			cur_page = 0;
		item_page->setIntValue(cur_page);
		if (page_stop)
			Pause();
	}
	
	protocole->charFromModem(buffer[cur_data++]);
//fprintf(stderr, "RecordModule::DoReplay() end\n");
}

//------------------------------------------------------------------------

void RecordModule::Play()
{
//fprintf(stderr, "RecordModule::Play() begin\n");
	if ((mode == kPLAY) || (buffer == NULL) || (nbr_data == 0))
		return;
	if (mode != kSTOP)
	{
		NXRunAlertPanel("Replay", "You can't replay a session while recording",
		"Ok", NULL, NULL);
		return;
	}
//	affiche_time_entry=DPSAddTimedEntry((double).0, (DPSTimedEntryProc)affTimeReplay, (void *)self, (int)NX_BASETHRESHOLD);
	Lock();
	contentView->SetFlags(contentView->Flags() | B_PULSE_NEEDED);
	SetPulseRate(100);
	Unlock();
	affiche_time_entry = 1;
	mode = kPLAY;
	nv = 0;
	loritel_win->orderFront();
//fprintf(stderr, "RecordModule::Play() end\n");
}

// file operations

//------------------------------------------------------------------------

void RecordModule::Open()
{
	if ((nbr_data != 0) || (mode != kSTOP))
	{
		NXRunAlertPanel("Open",
			"Data already exist.\nPlease make a 'New' before reading new ones.",
			"Ok", NULL, NULL);
		return;
	}
	be_app->RunFilePanel();
}

//------------------------------------------------------------------------

void RecordModule::DoOpen(record_ref *item)
{
	int32			i;
	int32			flag_eof
	int32			lu;
	unsigned char*	ptr;

	if (buffer == NULL)
		ReInit();
	if (buffer == NULL)
	{
		NXRunAlertPanel("Open", "No buffer\nReplay is impossible.", "Ok", NULL, NULL);
		return;
	}
	if ((nbr_data != 0) || (mode != kSTOP))
	{
		NXRunAlertPanel("Open",
			"Data already exist.\nPlease make a 'New' before reading new ones.",
			"Ok", NULL, NULL);
	}

	BFile file;
	file.SetRef(*item);
	if (file.OpenData() == B_NO_ERROR)
	{
		flag_eof = 0;
		nbr_data = 0;
		while (!flag_eof)
		{
			lu = (int) file.Read(&buffer[nbr_data], kPAS_ALLOC);
			nbr_data = nbr_data + lu;
			flag_eof = 1;
			if (lu >= kPAS_ALLOC)
			{
				ptr = (unsigned char *)realloc(buffer, max_alloc + kPAS_ALLOC);
				if (ptr == NULL)
				{
					NXRunAlertPanel("Open",
							"Buffer couldn't reallocate\nFile read aborted.",
							"Ok", NULL, NULL);
				}
				else
				{
					buffer = ptr;
					max_alloc = max_alloc + kPAS_ALLOC;
					flag_eof = 0;
				}
			}
		}

		file.CloseData();

		BDirectory	dd;
		file.GetParent(&dd);
		dir = dd.Record()->Ref();
//		strcpy(fic_save, file.Name());
		file.GetName(fic_save);

		for (i = 0; i < nbr_data; i++)
		{
			if (buffer[i] == 0x13)
			{
				page[max_page++] = i;
				if (max_page >= kMAX_PAGE)
					max_page = kMAX_PAGE - 1;
			}
		}
		
		cur_page = 0;
		cur_data = 0;
	}
	else
	{
		NXRunAlertPanel("Open", "File couldn't be opened.", "Ok", NULL, NULL);
	}	
}

//------------------------------------------------------------------------

void RecordModule::SetBinary()
{
	saveMode = 0;
	binSaveMI->SetMarked(TRUE);
	asciiSaveMI->SetMarked(FALSE);
/*	[savepanel setRequiredFileType:"loritel"];
	[scope_item setEnabled:NO];
	[a_item setEnabled:NO];
	[beg_item setEnabled:NO];
	[end_item setEnabled:NO]; */
}

//------------------------------------------------------------------------

void RecordModule::SetAscii()
{
	saveMode = 1;
	asciiSaveMI->SetMarked(TRUE);
	binSaveMI->SetMarked(FALSE);
/*	[savepanel setRequiredFileType:NULL];
	[scope_item setEnabled:YES];
	[scope_item selectCellAt:0:0];
	[self SetScopeAll:sender]; */
}

//------------------------------------------------------------------------

void RecordModule::SetScopeAll()
{
	savePart = 0;
/*	[a_item setEnabled:NO];
	[beg_item setEnabled:NO];
	[beg_item setIntValue:1];
	[end_item setEnabled:NO];
	[end_item setIntValue:max_page-1]; */
}

//------------------------------------------------------------------------

void RecordModule::SetScopePart()
{
	savePart = 1;
/*	[a_item setEnabled:YES];
	[beg_item setEnabled:YES];
	[end_item setEnabled:YES]; */
}

//------------------------------------------------------------------------

void RecordModule::EnterPage()
{
	int32			dummy;
	LRTextField*	sender = (LRTextField *)sender1;
	
	if (sender==beg_item)
	{
		dummy = sender->intValue();
		if (dummy < 1)
			dummy = 1;
		sender->setIntValue(dummy);
	}
	else
	{
		dummy = sender->intValue();
		if (dummy >= max_page)
			dummy = max_page - 1;
		sender->setIntValue(dummy);
	}
}

//------------------------------------------------------------------------

void RecordModule::DoSave()
{
	if (fic_save[0] == '\0')
		return;
	
	BDirectory	dd;
	BFile		file;

	dd.SetRef(dir);
	if (dd.GetFile(fic_save, &file) == B_NO_ERROR)	// already exists
	{
		;
	}
	else
	{
		if (dd.Create(fic_save, &file) != B_NO_ERROR)	// can't be created
		{
			NXRunAlertPanel("Save", "File couldn't be opened.", "Ok", NULL, NULL);
			return;
		}
	}

	app_info	info;
	be_app->GetAppInfo(&info);
	file.SetTypeAndApp('BDOC', info.signature);

	if (file.OpenData() == B_NO_ERROR)
	{
		file.Write(buffer, nbr_data);
		file.CloseData();
		modif_flag = 0;
	}
	else
		NXRunAlertPanel("Save Binary", "File couldn't be opened.", "Ok", NULL, NULL);
}

//------------------------------------------------------------------------

void RecordModule::DoSaveAscii()
{
	int32			oldpage;
	int32			olddispflag;
	int32			i;
	int32			l;
	int32			c;
	char			ligne[81];
	LoritelView*	bypass = (LoritelView *)ecran;
	
	if (fic_save[0] == '\0')
		return;
	
	BDirectory	dd;
	BFile		file;

	dd.SetRef(dir);
	if (dd.GetFile(fic_save, &file) == B_NO_ERROR)	// already exists
	{
		;
	}
	else
	{
		if (dd.Create(fic_save, &file) != B_NO_ERROR)	// can't be created
		{
			NXRunAlertPanel("Save", "File couldn't be opened.", "Ok", NULL, NULL);
			return;
		}
	}

	if (file.OpenData() == B_NO_ERROR)
	{
		oldpage = cur_page;
		olddispflag = autoDraw;
		autoDraw = 0;
		if (savePart)
		{
//			saveBeg = beg_item->intValue();
//			if (saveBeg<1)
				saveBeg = 1;
//			saveEnd = end_item->intValue();
//			if (saveEnd>=max_page)
				saveEnd = max_page - 1;
		}
		else
		{
			saveBeg = 1;
			saveEnd = max_page - 1;
		}
		for (i = saveBeg; i <= saveEnd; i++)
		{
			GoTo(i);
			for (l = 0; l <= 24; l++)
			{
				for (c = 1; c <= ecran->maxCol(); c++)
				{
					if (bypass->mem_page[l][c].pol < G1_NORM)
					{
						if (bypass->mem_page[l][c].car <32)
							ligne[c-1] = ' ';
						else
						{
							switch (bypass->mem_page[l][c].car)
							{
								case 0x60:
									ligne[c-1] = '­';
									break; /* middle bar */

								case 0x7b:
								case 0x7c:
								case 0x7d:
									ligne[c-1] = '|';
									break; /* vertical bar */

								case 0x7e:
									ligne[c-1] = '‰';
									break; /* upper bar */

								default:
									ligne[c-1] = bypass->mem_page[l][c].car;
									break;
							}
						}
					}
					else
						ligne[c-1] = ' ';
				}
				ligne[c-1] = '\n';
				ligne[c] = '\0';
				file.Write(ligne, c);
			}
			file.Write("\n", 1);
		}
		file.CloseData();
		autoDraw = 1;
		GoTo(oldpage);
		autoDraw = olddispflag;
	}
	else
		NXRunAlertPanel("Save Ascii", "File couldn't be opened.", "Ok", NULL, NULL);
	fic_save[0] = '\0';
}

//------------------------------------------------------------------------

void RecordModule::Save()
{
	if ((buffer == NULL) || (nbr_data == 0))
	{
		NXRunAlertPanel("Save", "No data to save.", "Ok", NULL, NULL);
		return;
	}
	
	if ((fic_save[0] == '\0') || (saveMode))
		SaveUnder(this);
	else
		SoSave();
}

//------------------------------------------------------------------------

void
RecordModule::SaveUnder()
{
	const char *file;
	
	if ((buffer == NULL) || (nbr_data == 0))
	{
		NXRunAlertPanel("Save", "No data to save.", "Ok", NULL, NULL);
		return;
	}
//	savepanel = [SavePanel new];
//	[savepanel setAccessoryView:saveView];
	if (fic_save[0])
		RunSavePanel(fic_save);
	else
		RunSavePanel("UNTITLED");

/*
	if (saveMode)
		SetAscii(this);
	else
		SetBinary(this);
	
	if ([savepanel runModal] == 1)
	{
		file = [savepanel filename];
		if (file == NULL)
			return;
		strcpy(fic_save, file);

		if (saveMode)
			DoSaveAscii();
		else
			DoSave();
	}
*/
}

//------------------------------------------------------------------------

void RecordModule::SaveRequested(record_ref directory, const char *filename)
{
	dir = directory;
	strcpy(fic_save, filename);
	if (saveMode)
		DoSaveAscii();
	else
		DoSave();
	CloseSavePanel();
}

//------------------------------------------------------------------------

void RecordModule::Close()
{	
	if (mode != kSTOP)
	{
		NXRunAlertPanel("New", "Sorry.\nYou can't do this when recording", "Ok", NULL, NULL);
		return;
	}
	
	if (modif_flag)
	{
		int32 rep = NXRunAlertPanel(NULL,
			"Unsaved recorded data.\nThey're going to be erased.",
			"Cancel", "Confirm erase", NULL);
		switch (rep)
		{
			case NX_ALERTALTERNATE: 
				break;

			default:
			case NX_ALERTDEFAULT: 
				return;
		}
	}
	
	if (buffer != NULL)
		free(buffer);
	cur_page = 0;
	page[0] = 0;
	max_page = 1;
	page_stop = 0;
	item_page->setIntValue(cur_page);

	nbr_data = 0;
	cur_data = 0;
	buffer = NULL;
	modif_flag = 0;
	fic_save[0] = '\0';
}
