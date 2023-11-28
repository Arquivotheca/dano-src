;
}

//--------------------------------------------------------------------

void TCDApplication::AboutRequested(void)
{
	char				about[32] = "...by Robert Polic";
	BAlert				*myAlert;

	myAlert = new BAlert("",about,"Big Deal");
	myAlert->Go();
}

//--------------------------------------------------------------------

void TCDApplication::ArgvReceived(int32 argc, char **argv)
{
	BEntry entry;

	if (entry.SetTo(argv[1]) == B_NO_ERROR) {
		BMessage msg(B_REFS_RECEIVED);
		entry_ref ref;

		entry.GetRef(&ref);
		msg.AddRef("refs", &ref);
		RefsReceived(&msg);
	}
}

//--------------------------------------------------------------------

void TCDApplication::MessageReceived(BMessage* msg)
{
	int32		index;
	BDirectory	dir;
	BEntry		entry;
	BFile		file;
	BPath		path;
	BPoint		win_pos;
	BRect		r;

	switch (msg->what) {
		case MENU_NEW:
			r.Set(BROWSER_WIND, TITLE_BAR_HEIGHT,
				  BROWSER_WIND + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);

			find_directory(B_USER_SETTINGS_DIRECTORY, &path);
			dir.SetTo(path.Path());
			if (dir.FindEntry("CDPlayer_data", &entry) == B_NO_ERROR) {
				file.SetTo(&entry, O_RDONLY);
				if (file.InitCheck() == B_NO_ERROR) {
					file.Read(&win_pos, sizeof(BPoint));
					win_pos.x += (10 * window_count);
					win_pos.y += (10 * window_count);
					if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(win_pos))
						r.OffsetTo(win_pos);
				}
			}
			index = msg->FindInt32("index");
	